//  $Id: challenge.cpp 1259 2007-09-24 12:28:19Z hiker $
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2008 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
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

#include "translation.hpp"
#include "challenges/challenge.hpp"
#include "world.hpp"
#include "race_manager.hpp"
#include "track_manager.hpp"
#if defined(WIN32) && !defined(__CYGWIN__)
#  define snprintf _snprintf
#endif


Challenge::Challenge(std::string id, std::string name) : 
    m_state(CH_INACTIVE), m_Id(id), m_Name(name)
{
}   // Challenge

//-----------------------------------------------------------------------------
/** Loads the state for a challenge object (esp. m_state), and calls the 
 *  virtual function loadState for additional information
 */
void Challenge::load(const lisp::Lisp* config)
{
    const lisp::Lisp* subnode= config->getLisp(getId());
    if(!subnode) return;
    
    // See if the challenge is solved (it's activated later from the
    // unlock_manager).
    bool finished=false;
    subnode->get("solved", finished);
    m_state = finished ? CH_SOLVED : CH_INACTIVE;
    if(!finished) loadState(subnode);
}   // load

//-----------------------------------------------------------------------------
void Challenge::save(lisp::Writer* writer)
{
    writer->beginList(getId());
    writer->write("solved", isSolved());
    if(!isSolved()) saveState(writer);
    writer->endList(getId());
}   // save

//-----------------------------------------------------------------------------
void Challenge::addUnlockTrackReward(std::string track_name)
{
    UnlockableFeature feature;
    feature.name = track_name;
    feature.type = UNLOCK_TRACK;
    m_feature.push_back(feature);
}
//-----------------------------------------------------------------------------
void Challenge::addUnlockModeReward(std::string internal_mode_name, std::string user_mode_name)
{
    UnlockableFeature feature;
    feature.name = internal_mode_name;
    feature.type = UNLOCK_MODE;
    feature.user_name = user_mode_name;
    m_feature.push_back(feature);
}
//-----------------------------------------------------------------------------
void Challenge::addUnlockGPReward(std::string gp_name)
{
    UnlockableFeature feature;
    feature.name = _(gp_name.c_str());
    feature.type = UNLOCK_GP;
    m_feature.push_back(feature);
}
//-----------------------------------------------------------------------------
void Challenge::addUnlockDifficultyReward(std::string internal_name, std::string user_name)
{
    UnlockableFeature feature;
    feature.name = internal_name;
    feature.type = UNLOCK_DIFFICULTY;
    feature.user_name = user_name;
    m_feature.push_back(feature);
}
//-----------------------------------------------------------------------------
const std::string Challenge::getUnlockedMessage() const
{
    std::string unlocked_message;
    
    const unsigned int amount = (unsigned int)m_feature.size();
    for(unsigned int n=0; n<amount; n++)
    {
        // add line break if we are showing multiple messages
        if(n>0) unlocked_message+='\n';
        
        char message[128];
        
        // write message depending on feature type
        switch(m_feature[n].type)
        {
            case UNLOCK_TRACK:
                {
                    Track* track = track_manager->getTrack( m_feature[n].name );
                    snprintf(message, 127, _("New track '%s'\nnow available"), gettext(track->getName()) );
                    break;
                }
            case UNLOCK_MODE:
                snprintf(message, 127, _("New game mode\n'%s'\nnow available"), m_feature[n].user_name.c_str() );
                break;
            case UNLOCK_GP:
                snprintf(message, 127, _("New Grand Prix '%s'\nnow available"), m_feature[n].name.c_str() );
                break;
            case UNLOCK_DIFFICULTY:
                snprintf(message, 127, _("New difficulty\n'%s'\nnow available"), m_feature[n].user_name.c_str() );
                break;
        }
        unlocked_message += message;
    }
    
    return unlocked_message;
}
