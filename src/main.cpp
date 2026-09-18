#include <dpp/dpp.h>
#include <iostream>
#include <exception>

// Include your subsystems
#include "SessionManager.h"
#include "CommandHandler.h"

// Make sure to paste your ACTUAL token from the Discord Developer Portal here!
const std::string BOT_TOKEN = "MTU0ODAxNzM0Nzc5NTc1MTAzMg.GhvRA0.1gyWHe2qhze2IWat097GSsUXehnDFOsegdU-u8";

const dpp::snowflake MY_SERVER_ID = 772126757192859648ULL;

int main() {
    try {
        dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
        bot.on_log(dpp::utility::cout_logger());

        SessionManager session_manager;

        bot.on_ready([&bot](const dpp::ready_t& event) {
            if (dpp::run_once<struct register_bot_commands>()) {
                dpp::slashcommand ping_cmd("ping", "Check bot latency", bot.me.id);
                dpp::slashcommand start_cmd("create_lobby", "Open a new game lobby", bot.me.id);
                dpp::slashcommand night_cmd("start_game", "Close lobby, assign roles, and begin Night 1", bot.me.id);
                bot.guild_bulk_command_create({ping_cmd, start_cmd, night_cmd}, MY_SERVER_ID);
            }
        });
        
        bot.on_slashcommand([&bot, &session_manager](const dpp::slashcommand_t& event) {
            std::string cmd = event.command.get_command_name();
            
            if (cmd == "ping") {
                event.reply("Pong!");
            }
            else if (cmd == "create_lobby") {
                CommandHandler::handle_create_lobby(event, session_manager);
            }
            else if (cmd == "start_game") {
                CommandHandler::handle_start_night(event, bot, session_manager);
            }
        });

        bot.on_button_click([&session_manager](const dpp::button_click_t& event) {
            CommandHandler::handle_button_clicks(event, session_manager);
        });

        bot.on_select_click([&bot, &session_manager](const dpp::select_click_t& event) {
            CommandHandler::handle_select_clicks(event, bot, session_manager);
        });

        bot.start(dpp::st_wait);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}