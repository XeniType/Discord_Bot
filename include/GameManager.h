#pragma once
#include <dpp/dpp.h>
#include <vector>
#include <algorithm>
#include <random>

// 1. Data Definitions
enum class GamePhase { IDLE, LOBBY, NIGHT, DAY, VOTING };
enum class Role { UNASSIGNED = 0, VILLAGER, MAFIA, DETECTIVE, DOCTOR };

struct Player {
    dpp::snowflake id;
    std::string username;
    Role role{Role::UNASSIGNED};
    bool is_alive{true};
    dpp::snowflake thread_id{0}; // For private threads for each player
};

struct NightResult {
    bool someone_died{false};
    dpp::snowflake victim_id{0};
    std::string victim_name;

    bool detective_investigated{false};
    dpp::snowflake detective_target_id{0};
    std::string detective_target_name;
    bool target_is_mafia{false};
};

// 2. Class Definition
class GameManager {
    private:
        GamePhase current_phase{GamePhase::IDLE};
        std::vector<Player> players;
        dpp::snowflake game_channel_id;
        dpp::snowflake host_id{0}; // track the host of the game

        dpp::snowflake mafia_target{0};
        dpp::snowflake doctor_target{0};
        dpp::snowflake detective_target{0};

    public:
        void set_player_thread(dpp::snowflake player_id, dpp::snowflake thread_id) {
            for (auto& p : players) {
                if (p.id == player_id) {
                    p.thread_id = thread_id;
                    break;
                }
            }
        }

        int get_required_night_actions_count() const {
            int count = 0;
            for (const auto& p : players) {
                if (p.is_alive && (p.role == Role::MAFIA || p.role == Role::DETECTIVE || p.role == Role::DOCTOR)) {
                    count++;
                }
            }
            return count;
        }

        int submitted_actions_count{0};

        bool submit_night_action(Role role, dpp::snowflake target_id) {
            if (current_phase != GamePhase::NIGHT) return false;
            
            if (role == Role::MAFIA) {
                mafia_target = target_id;
            } else if (role == Role::DOCTOR) {
                doctor_target = target_id;
            } else if (role == Role::DETECTIVE) {
                detective_target = target_id;
            } else {
                return false;
            }
            
            submitted_actions_count++;
            return true;
        }  

        bool start_next_night() {
            if (current_phase != GamePhase::DAY && current_phase != GamePhase::VOTING) return false;
            
            // Remove dead players from active role tracking or keep them as ghosts, 
            // but transition phase back to NIGHT
            current_phase = GamePhase::NIGHT;
            submitted_actions_count = 0;
            mafia_target = 0;
            doctor_target = 0;
            detective_target = 0;
            return true;
        }

        bool start_lobby(dpp::snowflake channel_id, dpp::snowflake user_id, const std::string& username) {
            if (current_phase != GamePhase::IDLE) return false;
            current_phase = GamePhase::LOBBY;
            game_channel_id = channel_id;
            host_id = user_id;
            players.clear();
            
            players.push_back(Player{user_id, username});
            return true;
        }

        bool add_player(dpp::snowflake user_id, const std::string& username) {
            if (current_phase != GamePhase::LOBBY) return false;
            for (const auto& p : players) {
                if (p.id == user_id) return false;
            }
            players.push_back(Player{user_id, username});
            return true;
        }

        bool remove_player(dpp::snowflake user_id) {
            if (current_phase != GamePhase::LOBBY) return false;
            auto it = std::remove_if(players.begin(), players.end(), [user_id](const Player& p) {
                return p.id == user_id;
            });

            if (it != players.end()) {
                players.erase(it, players.end());
                return true;
            }
            return false;
        }

        bool start_night() {
            if (current_phase != GamePhase::LOBBY) return false;
            if (players.size() < 3) return false; // Require at least 3 players

            current_phase = GamePhase::NIGHT;

            mafia_target = 0;
            doctor_target = 0;
            detective_target = 0;  

            // Shuffle players randomly
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(players.begin(), players.end(), g);

            // Assign roles based on player count
            players[0].role = Role::MAFIA;
            players[1].role = Role::DETECTIVE;
            if (players.size() >= 4) {
                players[2].role = Role::DOCTOR;
                for (size_t i = 3; i < players.size(); ++i) {
                    players[i].role = Role::VILLAGER;
                }
            } else {
                for (size_t i = 1; i < players.size(); ++i) {
                    players[i].role = Role::VILLAGER;
                }
            }
            return true;
        }

        void reset_game() {
            current_phase = GamePhase::IDLE;
            players.clear();
            host_id = 0;
        }

        NightResult resolve_night() {
            if (current_phase != GamePhase::NIGHT) return {};

            NightResult result;
            current_phase = GamePhase::DAY;

            // Check if Mafia killed someone and Doctor didn't save them
            if (mafia_target != 0 && mafia_target != doctor_target) {
                result.someone_died = true;
                result.victim_id = mafia_target;

                // Find victim name and set is_alive to false
                for (auto& p : players) {
                    if (p.id == mafia_target) {
                        p.is_alive = false;
                        result.victim_name = p.username;
                        break;
                    }
                }
            }

            if (detective_target != 0) {
            result.detective_investigated = true;
            result.detective_target_id = detective_target;

            for (const auto& p : players) {
                    if (p.id == detective_target) {
                        result.detective_target_name = p.username;
                        if (p.role == Role::MAFIA) {
                            result.target_is_mafia = true;
                        }
                        break;
                    }
                }
            }

            return result;
        }
        enum class Winner { NONE, TOWN, MAFIA };
        Winner check_win_condition() const {
            int mafia_alive = 0;
            int town_alive = 0;

            for (const auto& p : players) {
                if (p.is_alive) {
                    if (p.role == Role::MAFIA) mafia_alive++;
                    else town_alive++;
                }
            }

            if (mafia_alive == 0) return Winner::TOWN;
            if (mafia_alive >= town_alive) return Winner::MAFIA;
            return Winner::NONE;
        }

        std::string get_player_mentions() const {
            if (players.empty()) return "No players joined yet.";
            
            std::string list;
            for (const auto& p : players) {
                // Format: <@123456789>
                list += "<@" + std::to_string(p.id) + "> ";
            }
            return list;
        }

        Player get_player(dpp::snowflake id) {
            for (auto& p : players) {
                if (p.id == id) {
                    return p;
                }
            }
            return Player(); // Fallback if not found
        }

        dpp::snowflake get_host_id() const { return host_id; }
        GamePhase get_phase() const { return current_phase; }
        size_t get_player_count() const { return players.size(); }
        const std::vector<Player>& get_players() const { return players; }
}; // namespace dpp