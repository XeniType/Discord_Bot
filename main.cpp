#include <dpp/dpp.h>
#include <iostream>
#include <exception>

// Include your new local header file
#include "GameManager.h"

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
                dpp::slashcommand start_cmd("start_game", "Open a new game lobby", bot.me.id);
                bot.guild_bulk_command_create({ping_cmd, start_cmd}, MY_SERVER_ID);
            }
        });

        bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
            if (event.command.get_command_name() == "ping") {
                event.reply("Pong! Running natively in C++.");
            }
            else if (event.command.get_command_name() == "start_game") {
                if (game.start_lobby(event.command.channel_id)) {
                    // 1. Create the base message
                    dpp::message msg("Lobby opened! Click the button below to join.");
                    
                    // 2. Build the UI button and assign it a Custom ID
                    dpp::component button;
                    button.set_type(dpp::cot_button)
                        .set_style(dpp::cos_success)     // cos_success makes the button green
                        .set_label("Join Game")
                        .set_id("join_lobby_btn");       // We will listen for this ID later
                        
                    // 3. Add the button to an action row, and add the row to the message
                    msg.add_component(dpp::component().add_component(button));
                    
                    // 4. Send the enriched message
                    event.reply(msg);
                } else {
                    event.reply(dpp::message("A game is already in progress!").set_flags(dpp::m_ephemeral));
                }
            }
        });


        bot.on_button_click([](const dpp::button_click_t& event) {
                if (event.custom_id == "join_lobby_btn") {
        
                if (game.add_player(event.command.usr.id)) {
                    // 1. Construct the updated text using our new helper method
                    std::string updated_text = "Lobby opened! Click the button below to join.\n\n**Joined Players:**\n" 
                                            + game.get_player_mentions();
                    
                    dpp::message msg(updated_text);
                    
                    // 2. We must re-attach the button, because updating a message clears old components
                    dpp::component button;
                    button.set_type(dpp::cot_button)
                        .set_style(dpp::cos_success)
                        .set_label("Join Game")
                        .set_id("join_lobby_btn");
                        
                    msg.add_component(dpp::component().add_component(button));
                    
                    // 3. Use ir_update_message to edit the lobby live in the channel
                    event.reply(dpp::ir_update_message, msg);
                    
                } else {
                    // Player is already in the list, or the game already started
                    event.reply(dpp::message("You are already in the lobby!").set_flags(dpp::m_ephemeral));
                }
            }
        });
        bot.start(dpp::st_wait);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}