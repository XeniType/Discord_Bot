#pragma once
#include <dpp/dpp.h>
#include "SessionManager.h"

namespace CommandHandler{
    void handle_create_lobby(const dpp::slashcommand_t& event, SessionManager& sessions);
    void handle_start_night(const dpp::slashcommand_t& event, dpp::cluster& bot, SessionManager& sessions);
    void handle_button_clicks(const dpp::button_click_t& event, SessionManager& sessions);

    void handle_select_clicks(const dpp::select_click_t& event, dpp::cluster& bot, SessionManager& sessions);
    void execute_night_resolution(dpp::snowflake channel_id, dpp::snowflake guild_id, dpp::cluster& bot, SessionManager& sessions);
    void start_day_phase_timer(dpp::snowflake channel_id, dpp::cluster& bot, SessionManager& sessions);
}