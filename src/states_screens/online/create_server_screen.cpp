//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 Glenn De Jonghe
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "states_screens/online/create_server_screen.hpp"

#include "audio/sfx_manager.hpp"
#include "config/player_manager.hpp"
#include "config/user_config.hpp"
#include "network/network_config.hpp"
#include "network/server.hpp"
#include "network/server_config.hpp"
#include "network/socket_address.hpp"
#include "network/stk_host.hpp"
#include "online/online_profile.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/online/networking_lobby.hpp"
#include "utils/separate_process.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

#include <irrString.h>

#include <string>
#include <iostream>


using namespace GUIEngine;

// ----------------------------------------------------------------------------

CreateServerScreen::CreateServerScreen() : Screen("online/create_server.stkgui")
{
}   // CreateServerScreen

// ----------------------------------------------------------------------------

void CreateServerScreen::loadedFromFile()
{
    m_prev_mode = 0;
    m_prev_value = 0;
    m_name_widget = getWidget<TextBoxWidget>("name");
    assert(m_name_widget != NULL);
 
    m_max_players_widget = getWidget<SpinnerWidget>("max_players");
    assert(m_max_players_widget != NULL);
    int max = UserConfigParams::m_max_players.getDefaultValue();
    m_max_players_widget->setMax(max);

    if (UserConfigParams::m_max_players > max)
        UserConfigParams::m_max_players = max;

    m_max_players_widget->setValue(UserConfigParams::m_max_players);

    m_info_widget = getWidget<LabelWidget>("info");
    assert(m_info_widget != NULL);

    m_more_options_text = getWidget<LabelWidget>("more-options");
    assert(m_more_options_text != NULL);
    m_more_options_spinner = getWidget<SpinnerWidget>("more-options-spinner");
    assert(m_more_options_spinner != NULL);

    m_options_widget = getWidget<RibbonWidget>("options");
    assert(m_options_widget != NULL);
    m_game_mode_widget = getWidget<RibbonWidget>("gamemode");
    assert(m_game_mode_widget != NULL);
    m_create_widget = getWidget<IconButtonWidget>("create");
    assert(m_create_widget != NULL);
    m_cancel_widget = getWidget<IconButtonWidget>("cancel");
    assert(m_cancel_widget != NULL);
}   // loadedFromFile

// ----------------------------------------------------------------------------
void CreateServerScreen::init()
{
    Screen::init();
    m_supports_ai = NetworkConfig::get()->isLAN();
    m_info_widget->setText("", false);
    LabelWidget *title = getWidget<LabelWidget>("title");

    title->setText(NetworkConfig::get()->isLAN() ? _("Create LAN Server")
                                                 : _("Create Server")    ,
                   false);

    // I18n: Name of the server. %s is either the online or local user name
    m_name_widget->setText(_("%s's server",
                             NetworkConfig::get()->isLAN() 
                             ? PlayerManager::getCurrentPlayer()->getName()
                             : PlayerManager::getCurrentOnlineProfile()->getUserName()
                             )
                          );


    // -- Difficulty
    RibbonWidget* difficulty = getWidget<RibbonWidget>("difficulty");
    assert(difficulty != NULL);
    difficulty->setSelection(UserConfigParams::m_difficulty, PLAYER_ID_GAME_MASTER);

    // -- Game modes
    RibbonWidget* gamemode = getWidget<RibbonWidget>("gamemode");
    assert(gamemode != NULL);
    gamemode->setSelection(m_prev_mode, PLAYER_ID_GAME_MASTER);
    updateMoreOption(m_prev_mode);
}   // init

// ----------------------------------------------------------------------------
/** Event callback which starts the server creation process.
 */
void CreateServerScreen::eventCallback(Widget* widget, const std::string& name,
                                       const int playerID)
{
    if (name == m_options_widget->m_properties[PROP_ID])
    {
        const std::string& selection =
            m_options_widget->getSelectionIDString(PLAYER_ID_GAME_MASTER);
        if (selection == m_cancel_widget->m_properties[PROP_ID])
        {
            NetworkConfig::get()->unsetNetworking();
            StateManager::get()->escapePressed();
        }
        else if (selection == m_create_widget->m_properties[PROP_ID])
        {
            createServer();
        }   // is create_widget
    }
    else if (name == m_game_mode_widget->m_properties[PROP_ID])
    {
        const int selection =
            m_game_mode_widget->getSelection(PLAYER_ID_GAME_MASTER);
        m_prev_value = 0;
        updateMoreOption(selection);
        m_prev_mode = selection;
    }
    else if (name == m_max_players_widget->m_properties[PROP_ID] &&
        m_supports_ai)
    {
        m_prev_value = m_more_options_spinner->getValue();
        const int selection =
            m_game_mode_widget->getSelection(PLAYER_ID_GAME_MASTER);
        updateMoreOption(selection);
    }
}   // eventCallback

// ----------------------------------------------------------------------------
void CreateServerScreen::updateMoreOption(int game_mode)
{
    switch (game_mode)
    {
        case 0:
        case 1:
        {
            m_more_options_text->setVisible(true);
            m_more_options_spinner->setVisible(true);
            m_more_options_spinner->clearLabels();
            if (m_supports_ai)
            {
                m_more_options_text->setText(_("Number of AI karts"),
                    false);
                for (int i = 0; i <= m_max_players_widget->getValue() - 2; i++)
                {
                    m_more_options_spinner->addLabel(
                        StringUtils::toWString(i));
                }
                if (m_prev_value > m_max_players_widget->getValue() - 2)
                {
                    m_more_options_spinner->setValue(
                        m_max_players_widget->getValue() - 2);
                }
                else
                    m_more_options_spinner->setValue(m_prev_value);

            }
            else
            {
                //I18N: In the create server screen
                m_more_options_text->setText(_("No. of grand prix track(s)"),
                    false);
                m_more_options_spinner->addLabel(_("Disabled"));
                for (int i = 1; i <= 20; i++)
                {
                    m_more_options_spinner->addLabel(
                        StringUtils::toWString(i));
                }
                m_more_options_spinner->setValue(m_prev_value);
            }
            break;
        }
        case 2:
        {
            m_more_options_text->setVisible(true);
            m_more_options_spinner->setVisible(true);
            m_more_options_spinner->clearLabels();
            //I18N: In the create server screen, show various battle mode available
            m_more_options_text->setText(_("Battle mode"), false);
            m_more_options_spinner->setVisible(true);
            m_more_options_spinner->clearLabels();
            //I18N: In the create server screen for battle server
            m_more_options_spinner->addLabel(_("Free-For-All"));
            //I18N: In the create server screen for battle server
            m_more_options_spinner->addLabel(_("Capture The Flag"));
            m_more_options_spinner->setValue(m_prev_value);
            break;
        }
        case 3:
        {
            m_more_options_text->setVisible(true);
            m_more_options_spinner->setVisible(true);
            m_more_options_spinner->clearLabels();
            //I18N: In the create server screen
            m_more_options_text->setText(_("Soccer game type"), false);
            m_more_options_spinner->setVisible(true);
            m_more_options_spinner->clearLabels();
            //I18N: In the create server screen for soccer server
            m_more_options_spinner->addLabel(_("Time limit"));
            //I18N: In the create server screen for soccer server
            m_more_options_spinner->addLabel(_("Goals limit"));
            m_more_options_spinner->setValue(m_prev_value);
            break;
        }
        default:
        {
            m_more_options_text->setVisible(false);
            m_more_options_spinner->setVisible(false);
            break;
        }
    }
}   // updateMoreOption

// ----------------------------------------------------------------------------
/** Called once per framce to check if the server creation request has
 *  finished. If so, if pushes the server creation sceen.
 */
void CreateServerScreen::onUpdate(float delta)
{
    // If no host has been created, keep on waiting.
    if(!STKHost::existHost())
        return;

    NetworkingLobby::getInstance()->push();
}   // onUpdate

// ----------------------------------------------------------------------------
/** In case of WAN it adds the server to the list of servers. In case of LAN
 *  networking, it registers this game server with the stk server.
 */
void CreateServerScreen::createServer()
{
    const irr::core::stringw name = m_name_widget->getText().trim();
    const int max_players = m_max_players_widget->getValue();
    m_info_widget->setErrorColor();

    RibbonWidget* difficulty_widget = getWidget<RibbonWidget>("difficulty");
    RibbonWidget* gamemode_widget = getWidget<RibbonWidget>("gamemode");

    if (name.size() < 4 || name.size() > 30)
    {
        //I18N: In the create server screen
        m_info_widget->setText(
            _("Name has to be between 4 and 30 characters long!"), false);
        SFXManager::get()->quickSound("anvil");
        return;
    }
    assert(max_players > 1 && max_players <=
        UserConfigParams::m_max_players.getDefaultValue());

    UserConfigParams::m_max_players = max_players;
    std::string password = StringUtils::wideToUtf8(getWidget<TextBoxWidget>
        ("password")->getText());
    if ((!password.empty() != 0 &&
        password.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOP"
        "QRSTUVWXYZ01234567890_") != std::string::npos) ||
        password.size() > 255)
    {
        //I18N: In the create server screen
        m_info_widget->setText(
            _("Incorrect characters in password!"), false);
        SFXManager::get()->quickSound("anvil");
        return;
    }

    const bool private_server = !password.empty();
    ServerConfig::m_private_server_password = password;
    password = std::string(" --server-password=") + password;

    TransportAddress server_address(0x7f000001,
        stk_config->m_server_discovery_port);

    auto server = std::make_shared<Server>(0/*server_id*/, name,
        max_players, /*current_player*/0, (RaceManager::Difficulty)
        difficulty_widget->getSelection(PLAYER_ID_GAME_MASTER),
        0, server_address, private_server, false);

#undef USE_GRAPHICS_SERVER
#ifdef USE_GRAPHICS_SERVER
    NetworkConfig::get()->setIsServer(true);
    // In case of a WAN game, we register this server with the
    // stk server, and will get the server's id when this 
    // request is finished.
    ServerConfig::m_server_max_players = max_players;
    ServerConfig::m_server_name = StringUtils::xmlEncode(name);

    // FIXME: Add the following fields to the create server screen
    // FIXME: Long term we might add a 'vote' option (e.g. GP vs single race,
    // and normal vs FTL vs time trial could be voted about).
    std::string difficulty = difficulty_widget->getSelectionIDString(PLAYER_ID_GAME_MASTER);
    race_manager->setDifficulty(RaceManager::convertDifficulty(difficulty));
    race_manager->setMajorMode(RaceManager::MAJOR_MODE_SINGLE);

    std::string game_mode = gamemode_widget->getSelectionIDString(PLAYER_ID_GAME_MASTER);
    if (game_mode == "timetrial")
        race_manager->setMinorMode(RaceManager::MINOR_MODE_TIME_TRIAL);
    else
        race_manager->setMinorMode(RaceManager::MINOR_MODE_NORMAL_RACE);

    race_manager->setReverseTrack(false);
    auto sl = STKHost::create();
    assert(sl);
    sl->requestStart();
#else

    NetworkConfig::get()->setIsServer(false);
    std::ostringstream server_cfg;

    const std::string server_name = StringUtils::xmlEncode(name);
    if (NetworkConfig::get()->isWAN())
    {
        server_cfg << "--public-server --wan-server=" <<
            server_name << " --login-id=" <<
            NetworkConfig::get()->getCurrentUserId() << " --token=" <<
            NetworkConfig::get()->getCurrentUserToken();
    }
    else
    {
        server_cfg << "--lan-server=" << server_name;
    }

    // Clear previous stk-server-id-file_*
    std::set<std::string> files;
    const std::string server_id_file = "stk-server-id-file_";
    file_manager->listFiles(files, file_manager->getUserConfigDir());
    for (auto& f : files)
    {
        if (f.find(server_id_file) != std::string::npos)
        {
            file_manager->removeFile(file_manager->getUserConfigDir() + "/" +
                f);
        }
    }
    NetworkConfig::get()->setServerIdFile(
        file_manager->getUserConfigFile(server_id_file));

    server_cfg << " --stdout=server.log --mode=" <<
        gamemode_widget->getSelection(PLAYER_ID_GAME_MASTER) <<
        " --difficulty=" <<
        difficulty_widget->getSelection(PLAYER_ID_GAME_MASTER) <<
        " --max-players=" << max_players <<
        " --server-id-file=" << server_id_file <<
        " --log=1 --no-console-log";

    if (m_more_options_spinner->isVisible())
    {
        int esi = m_more_options_spinner->getValue();
        if (gamemode_widget->getSelection(PLAYER_ID_GAME_MASTER) ==
            3/*is soccer*/)
        {
            if (esi == 0)
                server_cfg << " --soccer-timed";
            else
                server_cfg << " --soccer-goals";
        }
        else if (gamemode_widget->getSelection(PLAYER_ID_GAME_MASTER) ==
            2/*is battle*/)
        {
            server_cfg << " --battle-mode=" << esi;
        }
        else
        {
            if (m_supports_ai)
            {
                if (esi > 0)
                    server_cfg << " --server-ai=" << esi;
            }
            else
            {
                // Grand prix track count
                if (esi > 0)
                    server_cfg << " --network-gp=" << esi;
            }
        }
        m_prev_mode = gamemode_widget->getSelection(PLAYER_ID_GAME_MASTER);
        m_prev_value = esi;
    }
    else
    {
        m_prev_mode = m_prev_value = 0;
    }

    SeparateProcess* sp =
        new SeparateProcess(SeparateProcess::getCurrentExecutableLocation(),
        server_cfg.str() + password);
    STKHost::create(sp);
    NetworkingLobby::getInstance()->setJoinedServer(server);
#endif
}   // createServer

// ----------------------------------------------------------------------------

void CreateServerScreen::tearDown()
{
}   // tearDown

