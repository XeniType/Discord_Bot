# Automated C++ Game Master Bot: Project Ledger

## What We Are Doing
Building a fully automated Social Deduction Game (like Mafia or Town of Salem) operating entirely within Discord. The bot acts as an unbiased Game Master, driven by a native C++ backend utilizing the D++ (`libdpp`) library. The architecture separates Discord networking logic from the core game rules, using a dedicated state machine to manage player entities, game phases, and hidden role interactions via Discord's interactive UI components.

## What We Did
* **Environment Setup:** Configured a CMake build pipeline that bypasses local package managers to pull the D++ library directly from GitHub via `FetchContent`.
* **Build Automation:** Wrote `POST_BUILD` CMake commands to automatically deploy required precompiled dependencies (like OpenSSL and Opus `.dll` files) alongside the MSVC executable.
* **Core Networking:** Established the asynchronous WebSocket connection to Discord's Gateway with robust `try/catch` exception handling for secure token validation.
* **State Machine Architecture:** Designed the `GameManager` class and `Player` struct in `include/GameManager.h` to track user IDs, roles, minimum player counts, and the current `GamePhase` (`IDLE`, `LOBBY`, `NIGHT`, `DAY`, `VOTING`).
* **Modular Codebase Refactor:** Cleaned up project organization by separating code into `include/` and `src/` directories, decoupling UI rendering (`UI.h` / `UI.cpp`) and state management from the core networking loop (`main.cpp`).
* **Advanced Lobby UI & Controls:** Developed an atmospheric dark crimson Discord Embed panel with action rows featuring three dynamic buttons: **Join Game**, **Leave Game**, and **Stop Lobby**, supporting real-time player list updates via mentions.
* **Phase Transition & Role Distribution:** Implemented the `start_night()` state transition logic in `GameManager` along with random shuffling (`std::mt19937`) and role assignment based on player count (Mafia, Detective, Doctor, Villager).
* **Secret Information Delivery:** Configured the `/start_night` slash command and integrated direct messaging (`bot.direct_message_create`) paired with styled ephemeral role cards (`UI.cpp`) to secretly distribute roles to active players.

## What We Need To Do Next
* **Night Action Subsystem:** Build role-specific interactive dropdown menus (`dpp::select_menu`) so active roles can secretly select their targets (e.g., who to investigate, who to eliminate).
* **Action Resolution & Day Phase:** Process the night's actions, resolve state changes (like setting a player's `is_alive` boolean to false), and announce the morning results in the public channel.
* **Dramatic Elimination System:** Program the physical voice channel disconnection mechanic to forcefully drop eliminated players from the call for maximum psychological impact.