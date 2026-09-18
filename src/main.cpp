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
dpp::snowflake main_lobby_channel_id = 0;

int main() {
    try {
        dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
        bot.on_log(dpp::utility::cout_logger());

        bot.on_ready([&bot](const dpp::ready_t& event) {
            if (dpp::run_once<struct register_bot_commands>()) {
                dpp::slashcommand ping_cmd("ping", "Check bot latency", bot.me.id);
                dpp::slashcommand start_cmd("create_lobby", "Open a new game lobby", bot.me.id);
                dpp::slashcommand night_cmd("start_night", "Close lobby, assign roles, and begin Night 1", bot.me.id);
                dpp::slashcommand next_night_cmd("next_night", "Proceed from day to the next night", bot.me.id);
                dpp::slashcommand resolve_cmd("resolve_night", "Force resolve the night", bot.me.id);
                bot.guild_bulk_command_create({ping_cmd, start_cmd, night_cmd, next_night_cmd, resolve_cmd}, MY_SERVER_ID);
            }
        });

        std::function<void()> start_day_phase_timer;
        std::function<void()> execute_night_resolution;

        start_day_phase_timer = [&bot]() {
            // Use main_lobby_channel_id instead of a passed parameter
            bot.message_create(dpp::message(main_lobby_channel_id, "☀️ **THE TOWN HAS 60 SECONDS TO DISCUSS AND ACCUSE.**\n*Night falls automatically when the timer expires.*"));

            bot.start_timer([&bot](const dpp::timer& timer_handle) {
                bot.stop_timer(timer_handle);

                if (game.start_next_night()) {
                    bot.message_create(dpp::message(main_lobby_channel_id, "🌙 **TIME IS UP. NIGHT FALLS ON THE TOWN ONCE MORE.**\n*Active roles, check your private threads to submit actions.*"));

                    // Re-use threads for Night 2, Night 3, etc.
                    for (const auto& player : game.get_players()) {
                        if (player.is_alive && player.thread_id != 0) {
                            if (player.role == Role::MAFIA || player.role == Role::DETECTIVE || player.role == Role::DOCTOR) {
                                std::string custom_id = "action_" + std::to_string(static_cast<int>(player.role));
                                std::string placeholder = "Choose your target...";
                                
                                dpp::message action_msg = UI::create_target_selection_message(game, player.role, custom_id, placeholder);
                                action_msg.channel_id = player.thread_id; // Send to thread!
                                bot.message_create(action_msg);
                            } else {
                                bot.message_create(dpp::message(player.thread_id, "🌙 Night has fallen. The town sleeps..."));
                            }
                        }
                    }
                }
            }, 60); // FIXED: Changed 120 to 60 to match the announcement
        };

        // 2. Night Resolution implementation
        execute_night_resolution = [&bot, &start_day_phase_timer]() {
            auto result = game.resolve_night();
            std::string announcement = "";

            if (result.someone_died) {
                announcement = "☀️ **THE SUN RISES...**\n\nThe town wakes up to a chilling discovery. **" + result.victim_name + 
                               "** was found lifeless in the square.\n\n*Their voice has been silenced.*";
            } else {
                announcement = "☀️ **THE SUN RISES...**\n\nA miraculous night! Everyone survived.";
            }

            GameManager::Winner winner = game.check_win_condition();
            if (winner != GameManager::Winner::NONE) {
                std::string win_announcement = (winner == GameManager::Winner::TOWN) ? 
                    "\n\n🎉 **TOWN WINS!** All mafia threats have been eliminated. Lobby reset!" : 
                    "\n\n🔪 **MAFIA WINS!** The shadows have overwhelmed the town. Lobby reset!";
                
                announcement += win_announcement;
                bot.message_create(dpp::message(main_lobby_channel_id, announcement));
                
                // NEW: Delete threads when game ends
                for (const auto& player : game.get_players()) {
                    if (player.thread_id != 0) bot.channel_delete(player.thread_id);
                }
                
                game.reset_game(); 
            } else {
                bot.message_create(dpp::message(main_lobby_channel_id, announcement));
                start_day_phase_timer(); 
            }
        };  

        bot.on_slashcommand([&bot, execute_night_resolution](const dpp::slashcommand_t& event) {
            std::string cmd = event.command.get_command_name();
            
            if (cmd == "ping") {
                event.reply("Pong!");
            }
            else if (cmd == "create_lobby") {
                if (game.start_lobby(event.command.channel_id, event.command.usr.id, event.command.usr.username)) {
                    main_lobby_channel_id = event.command.channel_id; // NEW: Save the lobby ID
                    event.reply(UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("A game is already in progress!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (cmd == "start_night") {
                if (game.start_night()) {
                    event.reply(UI::create_night_announcement_message());

                    // NEW: Create Persistent Threads on Night 1
                    for (const auto& player : game.get_players()) {
                        
                        // Use thread_create instead of channel_create
                        bot.thread_create(
                            player.username + " - Secret Role", // Thread name
                            main_lobby_channel_id,          // Parent channel ID
                            1440,                           // Auto-archive duration (1440 mins = 24 hours)
                            dpp::CHANNEL_PRIVATE_THREAD,    // Thread type
                            true,                           // Invitable
                            0,                              // Rate limit per user
                            [&bot, p_id = player.id](const dpp::confirmation_callback_t& callback) {
                                
                                // Error checking: if Discord blocks it, tell us why in the console!
                                if (callback.is_error()) {
                                    std::cerr << "FAILED TO CREATE THREAD: " << callback.get_error().message << "\n";
                                    return; // Stop right here
                                }

                                dpp::channel new_thread = callback.get<dpp::channel>();
                                game.set_player_thread(p_id, new_thread.id); // Save thread ID to the game manager

                                // Add player to thread
                                bot.thread_member_add(new_thread.id, p_id, [&bot, p_id, new_thread](const dpp::confirmation_callback_t& cb) {
                                    
                                    // Fetch the player so we know their role
                                    Player current_player = game.get_player(p_id); 
                                    
                                    // Send Role Card
                                    dpp::message role_msg = UI::create_role_card_message(current_player.role);
                                    role_msg.channel_id = new_thread.id;
                                    bot.message_create(role_msg);

                                    // Send Action Menu if they have an active role
                                    if (current_player.role == Role::MAFIA || current_player.role == Role::DETECTIVE || current_player.role == Role::DOCTOR) {
                                        std::string custom_id = "action_" + std::to_string(static_cast<int>(current_player.role));
                                        dpp::message action_msg = UI::create_target_selection_message(game, current_player.role, custom_id, "Choose your target...");
                                        action_msg.channel_id = new_thread.id;
                                        bot.message_create(action_msg);
                                    }
                                });
                            }
                        );
                    }
                } else {
                    event.reply(dpp::message("Cannot start night! Ensure you have an active lobby with at least 3 players.").set_flags(dpp::m_ephemeral));
                }
            }
            else if (cmd == "resolve_night") {
                execute_night_resolution();
                event.reply(dpp::message("Night resolved manually.").set_flags(dpp::m_ephemeral));
            }
        });

        bot.on_button_click([](const dpp::button_click_t& event) {
            if (event.custom_id == "join_lobby_btn") {
                if (game.add_player(event.command.usr.id, event.command.usr.username)) {
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

        bot.on_select_click([&bot, execute_night_resolution](const dpp::select_click_t& event) {
            if (event.values.empty()) return;
            dpp::snowflake target_id = std::stoull(event.values[0]);

            Role actor_role = Role::UNASSIGNED;
            if (event.custom_id == "action_1") actor_role = Role::MAFIA;
            else if (event.custom_id == "action_2") actor_role = Role::DETECTIVE; 
            else if (event.custom_id == "action_3") actor_role = Role::DOCTOR;

            if (game.submit_night_action(actor_role, target_id)) {
                event.reply(dpp::message("🔒 **Action locked in.** Your target has been recorded.").set_flags(dpp::m_ephemeral));

                if (game.submitted_actions_count >= game.get_required_night_actions_count()) {
                    execute_night_resolution(); // FIXED: Uses main_lobby_channel_id automatically now
                }
            } else {
                event.reply(dpp::message("Failed to record action. The night phase may have ended.").set_flags(dpp::m_ephemeral));
            }
        });

        bot.start(dpp::st_wait);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}