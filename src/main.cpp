#include <dpp/dpp.h>
#include <iostream>
#include <exception>

// Include your subsystems
#include "GameManager.h"
#include "UI.h"

// Make sure to paste your ACTUAL token from the Discord Developer Portal here!
const std::string BOT_TOKEN = "MTU0ODAxNzM0Nzc5NTc1MTAzMg.GhvRA0.1gyWHe2qhze2IWat097GSsUXehnDFOsegdU-u8";

const dpp::snowflake MY_SERVER_ID = {772126757192859648Ui64};

GameManager game;

int main() {
    try {
        dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
        bot.on_log(dpp::utility::cout_logger());

        bot.on_ready([&bot](const dpp::ready_t& event) {
            if (dpp::run_once<struct register_bot_commands>()) {
                dpp::slashcommand ping_cmd("ping", "Check bot latency", bot.me.id);
                dpp::slashcommand start_cmd("create_lobby", "Open a new game lobby", bot.me.id);
                dpp::slashcommand night_cmd("start_night", "Close lobby and begin the Night phase", bot.me.id);
                bot.guild_bulk_command_create({ping_cmd, start_cmd, night_cmd}, MY_SERVER_ID);
            }
        });

        bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
            if (event.command.get_command_name() == "ping") {
                event.reply("Pong! Running natively in C++.");
            }
            else if (event.command.get_command_name() == "create_lobby") {
                if (game.start_lobby(event.command.channel_id, event.command.usr.id)) {
                    // Cleanly handled by UI.h
                    event.reply(UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("A game is already in progress!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (event.command.get_command_name() == "start_night") {
                if (game.start_night()) {
                    // 1. Announce nightfall publicly in the channel using our UI helper
                    event.reply(UI::create_night_announcement_message());

                    // 2. Secretly distribute roles to each player
                    for (const auto& player : game.get_players()) {
                        // Send a private, ephemeral message or open a DM channel for each player
                        // For a seamless setup, we can use bot.direct_message_create or target them
                        bot.direct_message_create(player.id, UI::create_role_card_message(player.role), 
                            [](const dpp::confirmation_callback_t& callback) {
                                // Optional: handle if DMs are closed for a user
                            }
                        );
                    }
                } else {
                    event.reply(dpp::message("Cannot start night! Make sure you have an active lobby with at least 3 players.").set_flags(dpp::m_ephemeral));
                }
            }
        });

        bot.on_button_click([](const dpp::button_click_t& event) {
            if (event.custom_id == "join_lobby_btn") {
                if (game.add_player(event.command.usr.id)) {
                    event.reply(dpp::ir_update_message, UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("You are already in the lobby!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (event.custom_id == "leave_lobby_btn") {
                if (game.remove_player(event.command.usr.id)) {
                    event.reply(dpp::ir_update_message, UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("You aren't in the lobby yet!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (event.custom_id == "stop_lobby_btn") {
                game.reset_game();
                event.reply(dpp::ir_update_message, UI::create_cancelled_lobby_message());
            }
        });

        bot.start(dpp::st_wait);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}