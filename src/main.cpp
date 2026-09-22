#include <dpp/dpp.h>
#include <cstdlib>
#include <iostream>
#include <exception>

#include "SessionManager.h"
#include "CommandHandler.h"
#include "dotenv.h"

int main() {
    dotenv env(".env"); // Load environment variables from .env file
    std::string token_env = env.get("DISCORD_TOKEN");
    
    if (token_env.empty()) {
        std::cerr << "Error: DISCORD_TOKEN environment variable not set!\n";
        return 1;
    }

    try {
        dpp::cluster bot(token_env, dpp::i_default_intents | dpp::i_message_content);
        bot.on_log(dpp::utility::cout_logger());

        SessionManager session_manager;

        bot.on_ready([&bot](const dpp::ready_t& event) {
            if (dpp::run_once<struct register_bot_commands>()) {
                dpp::slashcommand ping_cmd("ping", "Check bot latency", bot.me.id);
                dpp::slashcommand start_cmd("create_lobby", "Open a new game lobby", bot.me.id);
                dpp::slashcommand night_cmd("start_game", "Close lobby, assign roles, and begin Night 1", bot.me.id);
                
                bot.global_bulk_command_create({ping_cmd, start_cmd, night_cmd});
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