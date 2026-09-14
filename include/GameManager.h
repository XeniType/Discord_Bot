#pragma once
#include <dpp/dpp.h>
#include <vector>
#include <algorithm>
#include <random>

// 1. Data Definitions
enum class GamePhase { IDLE, LOBBY, NIGHT, DAY, VOTING };
enum class Role { UNASSIGNED, VILLAGER, MAFIA, DETECTIVE, DOCTOR };

struct Player {
    dpp::snowflake id;
    Role role{Role::UNASSIGNED};
    bool is_alive{true};
};

// 2. Class Definition
class GameManager {
    private:
        GamePhase current_phase{GamePhase::IDLE};
        std::vector<Player> players;
        dpp::snowflake game_channel_id;
        dpp::snowflake host_id{0}; // track the host of the game

    public:
        bool start_lobby(dpp::snowflake channel_id, dpp::snowflake user_id) {
            if (current_phase != GamePhase::IDLE) return false;
            current_phase = GamePhase::LOBBY;
            game_channel_id = channel_id;
            host_id = user_id; // Set the host ID when starting the lobby
            players.clear();
            
            players.push_back(Player{user_id}); // Add the host as the first player
            return true;
        }

        bool add_player(dpp::snowflake user_id) {
            if (current_phase != GamePhase::LOBBY) return false;
            for (const auto& p : players) {
                if (p.id == user_id) return false;
            }
            players.push_back(Player{user_id});
            return true;
        }

        bool remove_player(dpp::snowflake user_id) {
            if (current_phase != GamePhase::LOBBY) return false;
            
            auto it = std::remove_if(players.begin(), players.end(), [user_id](const Player& p) {
                return p.id == user_id;
            });

            if (it != players.end()) {
                players.erase(it, players.end());
                return true; // Successfully removed
            }
            return false; // Player wasn't in the lobby
        }

        bool start_night() {
            if (current_phase != GamePhase::LOBBY) return false;
            //if (players.size() < 3) return false; // Require at least 3 players

            current_phase = GamePhase::NIGHT;

            // Shuffle players randomly
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(players.begin(), players.end(), g);

            // Assign roles based on player count
            players[0].role = Role::MAFIA;
           /* 
           players[1].role = Role::DETECTIVE;
           if (players.size() >= 4) {
                players[2].role = Role::DOCTOR;
                for (size_t i = 3; i < players.size(); ++i) {
                    players[i].role = Role::VILLAGER;
                }
            } else {
                for (size_t i = 2; i < players.size(); ++i) {
                    players[i].role = Role::VILLAGER;
                }
            }
            */ 
            return true;
        }

        void reset_game() {
            current_phase = GamePhase::IDLE;
            players.clear();
            host_id = 0;
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

        dpp::snowflake get_host_id() const { return host_id; }
        GamePhase get_phase() const { return current_phase; }
        size_t get_player_count() const { return players.size(); }
        const std::vector<Player>& get_players() const { return players; }
}; // namespace dpp