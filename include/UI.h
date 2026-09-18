#pragma once
#include <dpp/dpp.h>
#include "GameManager.h"

namespace UI {
    // Lobby UI
    dpp::message create_lobby_message(const GameManager& game);
    dpp::message create_cancelled_lobby_message();

    // Night Phase UI (For announcing nightfall and private role cards)
    dpp::message create_night_announcement_message();
    dpp::message create_role_card_message(Role role);

    // Generates a select menu containing all alive players as choices
    dpp::message create_target_selection_message(const GameManager& game, Role viewer_role, const std::string& custom_id, const std::string& placeholder);
} // namespace UI
