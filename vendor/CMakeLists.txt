# Hermetic dependency management
#
# This file orchestrates third-party dependencies via FetchContent or
# git submodules. All libraries are made available as targets that can
# be linked by dltchat_app_logic.
#
# Policy: prefer FetchContent with pinned versions over system packages.
#         Only use find_package for Host-required deps (Qt, qdlt).

include(FetchContent)

# ── nlohmann_json (header-only) ──────────────────────────────────
# Only if needed; currently the project uses Qt's JSON classes.
# Uncomment and pin the version when JSON parsing beyond Qt is needed.
#
# FetchContent_Declare(nlohmann_json
#     GIT_REPOSITORY https://github.com/nlohmann/json.git
#     GIT_TAG v3.11.3
#     GIT_SHALLOW TRUE
# )
# FetchContent_MakeAvailable(nlohmann_json)

# ── fmtlib (optional, for string formatting) ────────────────────
# FetchContent_Declare(fmt
#     GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#     GIT_TAG 10.2.1
#     GIT_SHALLOW TRUE
# )
# FetchContent_MakeAvailable(fmt)

message(STATUS "vendor: no external dependencies configured (all deps come from Qt + qdlt)")
