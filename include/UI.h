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
} // namespace UI
