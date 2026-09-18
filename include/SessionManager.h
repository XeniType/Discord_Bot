#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <dpp/dpp.h>
#include <utility>
#include "GameManager.h"

const int MAX_ACTIVE_SESSIONS = 4;

class SessionManager {
private:
    std::unordered_map<dpp::snowflake, std::shared_ptr<GameManager>> active_sessions;
    std::mutex session_mutex; // Essential for thread safety in D++ async callbacks

public:
    std::pair<dpp::snowflake, std::shared_ptr<GameManager>> get_session_by_interaction(dpp::snowflake interaction_channel_id) {
        std::lock_guard<std::mutex> lock(session_mutex);
        
        // 1. Check if the interaction happened in the main lobby channel
        auto it = active_sessions.find(interaction_channel_id);
        if (it != active_sessions.end()) {
            return {it->first, it->second};
        }

        // 2. If not found, check if it happened inside a player's private thread
        for (const auto& [main_channel_id, game] : active_sessions) {
            for (const auto& player : game->get_players()) {
                if (player.thread_id == interaction_channel_id) {
                    // Return the MAIN lobby ID alongside the game session
                    return {main_channel_id, game};
                }
            }
        }

        return {0, nullptr}; // Not found
    }

    std::shared_ptr<GameManager> create_session(dpp::snowflake channel_id) {
        std::lock_guard<std::mutex> lock(session_mutex);
        if(active_sessions.size() >= MAX_ACTIVE_SESSIONS) {
            return nullptr;
        }
        if (active_sessions.find(channel_id) != active_sessions.end()) {
            return nullptr; 
        }
        auto new_game = std::make_shared<GameManager>();
        active_sessions[channel_id] = new_game;
        return new_game;
    }

    std::shared_ptr<GameManager> get_session(dpp::snowflake channel_id) {
        std::lock_guard<std::mutex> lock(session_mutex);
        auto it = active_sessions.find(channel_id);
        if (it != active_sessions.end()) {
            return it->second;
        }
        return nullptr;
    }

    void end_session(dpp::snowflake channel_id) {
        std::lock_guard<std::mutex> lock(session_mutex);
        active_sessions.erase(channel_id);
    }
};