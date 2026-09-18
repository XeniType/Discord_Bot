#include "UI.h"

namespace UI {
    
    dpp::message create_lobby_message(const GameManager& game) {
        dpp::embed embed;
        embed.set_color(0x8B0000) // Deep Crimson
        .set_title("🌙 THE TOWN SQUARE — GATHERING")
        .set_description("Whispers echo through the cobblestone streets. The town is uneasy as darkness approaches...\n\n*Click **Join Game** to secure your place before night falls.*")
        .add_field("Active Conspirators (" + std::to_string(game.get_player_count()) + ")", 
        game.get_player_mentions(), false)
        .set_footer(dpp::embed_footer().set_text("Engine v0.1 • Native C++ Backend"));
        
        dpp::component join_btn;
        join_btn.set_type(dpp::cot_button).set_style(dpp::cos_success).set_label("Join Game").set_id("join_lobby_btn");
        
        dpp::component leave_btn;
        leave_btn.set_type(dpp::cot_button).set_style(dpp::cos_secondary).set_label("Leave Game").set_id("leave_lobby_btn");
        
        dpp::component stop_btn;
        stop_btn.set_type(dpp::cot_button).set_style(dpp::cos_danger).set_label("Stop Lobby").set_id("stop_lobby_btn");
        
        dpp::component action_row;
        action_row.set_type(dpp::cot_action_row);
        action_row.add_component(join_btn);
        action_row.add_component(leave_btn);
        action_row.add_component(stop_btn);
        
        dpp::message msg;
        msg.add_embed(embed);
        msg.add_component(action_row);
        return msg;
    }
    
    dpp::message create_cancelled_lobby_message() {
        dpp::embed embed;
        embed.set_color(0x36393F)
        .set_title("🚫 LOBBY CANCELLED")
        .set_description("The gathering has been called off by the host.");
        
        dpp::message msg;
        msg.add_embed(embed);
        return msg;
    }
    
    // NEW: Night phase public broadcast embed
    dpp::message create_night_announcement_message() {
        dpp::embed embed;
        embed.set_color(0x0B0B45) // Dark Midnight Blue
        .set_title("🌑 NIGHT HAS FALLEN")
        .set_description("A heavy fog rolls over the town. Everyone goes to sleep... but the shadows stir.\n\n*Active roles, check your private alerts to perform your night actions.*");
        
        dpp::message msg;
        msg.add_embed(embed);
        return msg;
    }
    
    // NEW: Secret role card embed sent privately to each player
    dpp::message create_role_card_message(Role role) {
        dpp::embed embed;
        std::string role_title;
        std::string role_desc;
        uint32_t embed_color = 0x2F3136;
        
        switch (role) {
            case Role::MAFIA:
            role_title = "🔪 SECRET ROLE: MAFIA";
            role_desc = "You are part of the shadow syndicate. Coordinate with your allies to eliminate the town before dawn.";
            embed_color = 0x990000; // Blood Red
            break;
            case Role::DETECTIVE:
            role_title = "🔍 SECRET ROLE: DETECTIVE";
            role_desc = "You have a sharp eye for deceit. Investigate one person each night to uncover their true allegiance.";
            embed_color = 0x1F8B4C; // Emerald Green
            break;
            case Role::DOCTOR:
            role_title = "💉 SECRET ROLE: DOCTOR";
            role_desc = "You carry medical supplies to protect the innocent. Choose one person to shield from danger each night.";
            embed_color = 0x3498DB; // Dodger Blue
            break;
            default:
            role_title = "🛡️ SECRET ROLE: VILLAGER";
            role_desc = "You are a peaceful citizen. Work together during the day discussions to find out who the mafia are!";
            embed_color = 0x95A5A6; // Gray
            break;
        }
        
        embed.set_color(embed_color)
        .set_title(role_title)
        .set_description(role_desc);
        
        dpp::message msg;
        msg.set_flags(dpp::m_ephemeral); // Keep it private
        msg.add_embed(embed);
        return msg;
    }

    dpp::message create_target_selection_message(const GameManager& game, Role viewer_role, const std::string& custom_id, const std::string& placeholder) {
        dpp::embed embed;
        embed.set_color(0x2F3136)
            .set_title("🎯 CHOOSE YOUR TARGET")
            .set_description("Select a player from the dropdown menu below to perform your night action.");

        dpp::component select_menu;
        select_menu.set_type(dpp::cot_selectmenu)
                .set_id(custom_id)
                .set_placeholder(placeholder)
                .set_min_values(1)
                .set_max_values(1);

        for (const auto& player : game.get_players()) {
            if (player.is_alive) {
                // Rule: If the viewer is Mafia, do not show other Mafia members as targets
                if (viewer_role == Role::MAFIA && player.role == Role::MAFIA) {
                    continue; 
                }

                select_menu.add_select_option(dpp::select_option(
                    player.username,             // Displays actual username
                    std::to_string(player.id), // Hidden value contains user ID
                    "Target " + player.username
                ));
            }
        }

        dpp::component action_row;
        action_row.set_type(dpp::cot_action_row);
        action_row.add_component(select_menu);

        dpp::message msg;
        msg.set_flags(dpp::m_ephemeral);
        msg.add_embed(embed);
        msg.add_component(action_row);
        return msg;
    }
} // namespace UI