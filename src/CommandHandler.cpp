#include "CommandHandler.h"
#include "UI.h"

namespace CommandHandler {
    void handle_create_lobby(const dpp::slashcommand_t& event, SessionManager& session_manager) {
        dpp::snowflake channel_id = event.command.channel_id;

        auto game = session_manager.create_session(channel_id);

        if (game) {
            game->start_lobby(channel_id, event.command.usr.id, event.command.usr.username);
            event.reply(UI::create_lobby_message(game));
        }
    }

    void handle_start_night(const dpp::slashcommand_t& event, dpp::cluster& bot, SessionManager& session_manager) {
        dpp::snowflake channel_id = event.command.channel_id;
        auto game = session_manager.get_session(channel_id);

        if (!game) {
            event.reply(dpp::message("No active game found here.").set_flags(dpp::m_ephemeral));
            return;
        }

        // RESTORED: The missing if-statement
        if (game->start_night()) {
            event.reply(UI::create_night_announcement_message());

            for (const auto& player : game->get_players()) {
                bot.thread_create(
                    player.username + " - Secret Role", 
                    channel_id, // REPLACED: main_lobby_channel_id with the local channel_id
                    1440, dpp::CHANNEL_PRIVATE_THREAD, true, 0,
                    
                    // ADDED: Capture 'game' in the lambda
                    [&bot, p_id = player.id, game](const dpp::confirmation_callback_t& callback) {
                        if (callback.is_error()) {
                            std::cerr << "FAILED TO CREATE THREAD: " << callback.get_error().message << "\n";
                            return;
                        }

                        dpp::thread new_thread = callback.get<dpp::thread>(); 
                        game->set_player_thread(p_id, new_thread.id); // FIXED: Used ->

                        // ADDED: Capture 'game' again for the inner lambda
                        bot.thread_member_add(new_thread.id, p_id, [&bot, p_id, new_thread, game](const dpp::confirmation_callback_t& cb) {
                            Player current_player = game->get_player(p_id); // FIXED: Used ->
                            
                            dpp::message role_msg = UI::create_role_card_message(current_player.role);
                            role_msg.channel_id = new_thread.id;
                            bot.message_create(role_msg);

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
            event.reply(dpp::message("Cannot start night! Ensure you have an active lobby with enough players.").set_flags(dpp::m_ephemeral));
        }
    }

    void handle_button_clicks(const dpp::button_click_t& event, SessionManager& sessions) {
        dpp::snowflake channel_id = event.command.channel_id;
        auto game = sessions.get_session(channel_id);

        if (!game) {
            event.reply(dpp::message("No active game found here.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (event.custom_id == "join_lobby_btn") {
                if (game->add_player(event.command.usr.id, event.command.usr.username)) {
                    event.reply(dpp::ir_update_message, UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("You are already in the lobby!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (event.custom_id == "leave_lobby_btn") {
                if (game->remove_player(event.command.usr.id)) {
                    event.reply(dpp::ir_update_message, UI::create_lobby_message(game));
                } else {
                    event.reply(dpp::message("You aren't in the lobby yet!").set_flags(dpp::m_ephemeral));
                }
            }
            else if (event.custom_id == "stop_lobby_btn") {
                game->reset_game();
                event.reply(dpp::ir_update_message, UI::create_cancelled_lobby_message());
            }
    }

    void start_day_phase_timer(dpp::snowflake channel_id, dpp::cluster& bot, SessionManager& sessions) {
        bot.message_create(dpp::message(channel_id, "☀️ **THE TOWN HAS 60 SECONDS TO DISCUSS AND ACCUSE.**\n*Night falls automatically when the timer expires.*"));

        bot.start_timer([&bot, &sessions, channel_id](const dpp::timer& timer_handle) {
            bot.stop_timer(timer_handle);

            auto game = sessions.get_session(channel_id);
            if (!game) return; // Failsafe if game was cancelled

            if (game->start_next_night()) {
                bot.message_create(dpp::message(channel_id, "🌙 **TIME IS UP. NIGHT FALLS ON THE TOWN ONCE MORE.**\n*Active roles, check your private threads to submit actions.*"));

                // Re-use threads for Night 2, Night 3, etc.
                for (const auto& player : game->get_players()) {
                    if (player.is_alive && player.thread_id != 0) {
                        if (player.role == Role::MAFIA || player.role == Role::DETECTIVE || player.role == Role::DOCTOR) {
                            std::string custom_id = "action_" + std::to_string(static_cast<int>(player.role));
                            dpp::message action_msg = UI::create_target_selection_message(game, player.role, custom_id, "Choose your target...");
                            action_msg.channel_id = player.thread_id; 
                            bot.message_create(action_msg);
                        } else {
                            bot.message_create(dpp::message(player.thread_id, "🌙 Night has fallen. The town sleeps..."));
                        }
                    }
                }
            }
        }, 60); 
    }

    void execute_night_resolution(dpp::snowflake channel_id, dpp::snowflake guild_id, dpp::cluster& bot, SessionManager& sessions) {
        auto game = sessions.get_session(channel_id);
        if (!game) return;

        auto result = game->resolve_night();
        std::string announcement = "";
        if (result.someone_died) {
            // DISCONNECT THE PLAYER FROM VOICE CHAT
            // Passing 0 as the channel_id drops them from the voice call entirely.
            bot.guild_member_move(0, guild_id, result.victim_id);

            announcement = "☀️ **THE SUN RISES...**\n\nThe town wakes up to a chilling discovery. **" + 
                        result.victim_name + "** was found lifeless in the square.\n\n*Their voice has been silenced.*";
        } else {
            announcement = "☀️ **THE SUN RISES...**\n\nA miraculous night! Everyone survived.";
        }

        if (result.detective_investigated) {
            for (const auto& player : game->get_players()) {
                if (player.role == Role::DETECTIVE && player.thread_id != 0) {
                    std::string verdict = result.target_is_mafia ? 
                        "🚨 **SUSPICIOUS!** Your investigation reveals that **" + result.detective_target_name + "** is part of the Mafia!" : 
                        "🛡️ **INNOCENT.** Your investigation reveals that **" + result.detective_target_name + "** appears to be a peaceful town citizen.";
                    
                    bot.message_create(dpp::message(player.thread_id, verdict));
                    break;
                }
            }
        }
            GameManager::Winner winner = game->check_win_condition();
        if (winner != GameManager::Winner::NONE) {
            std::string win_announcement = (winner == GameManager::Winner::TOWN) ? 
                "\n\n🎉 **TOWN WINS!** All mafia threats have been eliminated. Lobby reset!" : 
                "\n\n🔪 **MAFIA WINS!** The shadows have overwhelmed the town. Lobby reset!";
            
            announcement += win_announcement;
            bot.message_create(dpp::message(channel_id, announcement));
            
            // Delete threads when game ends
            for (const auto& player : game->get_players()) {
                if (player.thread_id != 0) bot.channel_delete(player.thread_id);
            }
            
            sessions.end_session(channel_id); 
        } else {
            bot.message_create(dpp::message(channel_id, announcement));
            start_day_phase_timer(channel_id, bot, sessions); // Transition smoothly into the day timer
        }
    }

    void handle_select_clicks(const dpp::select_click_t& event, dpp::cluster& bot, SessionManager& sessions) {
        if (event.values.empty()) return;
    
        // CHANGED: Use the new smart lookup instead of get_session
        auto [main_channel_id, game] = sessions.get_session_by_interaction(event.command.channel_id);
        
        if (!game) {
            event.reply(dpp::message("No active game found.").set_flags(dpp::m_ephemeral));
            return;
        }

        dpp::snowflake target_id = std::stoull(event.values[0]);
        Role actor_role = Role::UNASSIGNED;
        
        if (event.custom_id == "action_2") actor_role = Role::MAFIA;
        else if (event.custom_id == "action_3") actor_role = Role::DETECTIVE; 
        else if (event.custom_id == "action_4") actor_role = Role::DOCTOR;

        if (game->submit_night_action(actor_role, target_id)) {
            event.reply(dpp::message("🔒 **Action locked in.**").set_flags(dpp::m_ephemeral));

            if (game->submitted_actions_count >= game->get_required_night_actions_count()) {
                // PASS THE MAIN CHANNEL ID here, so the sun rises in the correct channel!
                execute_night_resolution(main_channel_id, event.command.guild_id, bot, sessions); 
            }
        } else {
            event.reply(dpp::message("Failed to record action.").set_flags(dpp::m_ephemeral));
        }
    }
}