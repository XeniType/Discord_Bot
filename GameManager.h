#pragma once
#include <dpp/dpp.h>
#include <vector>

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

public:
    bool start_lobby(dpp::snowflake channel_id) {
        if (current_phase != GamePhase::IDLE) return false;
        current_phase = GamePhase::LOBBY;
        game_channel_id = channel_id;
        players.clear();
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

    GamePhase get_phase() const { return current_phase; }
    size_t get_player_count() const { return players.size(); }

    std::string get_player_mentions() const {
        if (players.empty()) return "No players joined yet.";
        
        std::string list;
        for (const auto& p : players) {
            // Format: <@123456789>
            list += "<@" + std::to_string(p.id) + "> ";
        }
        return list;
    }
};