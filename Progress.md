# Automated C++ Game Master Bot: Project Ledger

## What We Are Doing
Building a fully automated Social Deduction Game (like Mafia or Town of Salem) operating entirely within Discord. The bot acts as an unbiased Game Master, driven by a native C++ backend utilizing the D++ (`libdpp`) library. The architecture separates Discord networking logic from the core game rules, using a dedicated state machine to manage player entities, game phases, and hidden role interactions via Discord's interactive UI components.

## What We Did
* **Environment Setup:** Configured a CMake build pipeline that bypasses local package managers to pull the D++ library directly from GitHub via `FetchContent`.
* **Build Automation:** Wrote `POST_BUILD` CMake commands to automatically deploy required precompiled dependencies (like OpenSSL and Opus `.dll` files) alongside the MSVC executable.
* **Core Networking:** Established the asynchronous WebSocket connection to Discord's Gateway with robust `try/catch` exception handling for secure token validation.
* **State Machine Architecture:** Designed the `GameManager` class and `Player` struct in `GameManager.h` to track user IDs, roles, and the current `GamePhase` (`IDLE`, `LOBBY`, `NIGHT`, `DAY`, `VOTING`).
* **Guild Command Syncing:** Implemented `guild_bulk_command_create` to instantly register and sync Slash Commands (`/ping`, `/start_game`) to the testing server, avoiding Discord's global cache delays.
* **Interactive UI & Live State:** Built an active lobby system that sends a "Join Game" UI button component. Fixed global variable lambda captures to safely process `bot.on_button_click` events, using `dpp::ir_update_message` to dynamically append new players to the lobby message using their Discord `<@ID>` mentions.

## What We Need To Do Next
* **Phase Transition Logic:** Create a command (e.g., `/start_night`) that locks the lobby, rejects new joins, and transitions the `GameManager` state from `LOBBY` to `NIGHT`.
* **Role Distribution:** Write the logic to shuffle the `players` vector and assign specific roles (Mafia, Detective, Doctor, Villager) based on the total player count.
* **Secret Information Delivery:** Implement Direct Messages (DMs) or targeted Ephemeral Messages to secretly inform each connected player of their assigned role and win conditions.
* **Night Action Subsystem:** Build role-specific interactive dropdown menus (`dpp::select_menu`) so active roles can secretly select their targets (e.g., who to investigate, who to eliminate).
* **Action Resolution & Day Phase:** Process the night's actions, resolve state changes (like setting a player's `is_alive` boolean to false), and announce the morning results in the public channel.