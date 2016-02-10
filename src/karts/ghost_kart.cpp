//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2012-2015  Joerg Henrichs
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

#include "karts/ghost_kart.hpp"
#include "karts/controller/ghost_controller.hpp"
#include "karts/kart_gfx.hpp"
#include "karts/kart_model.hpp"
#include "modes/world.hpp"

#include "LinearMath/btQuaternion.h"

GhostKart::GhostKart(const std::string& ident, unsigned int world_kart_id,
                     int position)
          : Kart(ident, world_kart_id,
                 position, btTransform(btQuaternion(0, 0, 0, 1)),
                 PLAYER_DIFFICULTY_NORMAL)
{
}   // GhostKart

// ----------------------------------------------------------------------------
void GhostKart::reset()
{
    m_node->setVisible(true);
    Kart::reset();
    // This will set the correct start position
    update(0);
}   // reset

// ----------------------------------------------------------------------------
void GhostKart::addReplayEvent(float time,
                               const btTransform &trans,
                               const ReplayBase::PhysicInfo &pi,
                               const ReplayBase::KartReplayEvent &kre)
{
    GhostController* gc = dynamic_cast<GhostController*>(getController());
    gc->addReplayTime(time);

    m_all_transform.push_back(trans);
    m_all_physic_info.push_back(pi);
    m_all_replay_events.push_back(kre);

    // Use first frame of replay to calculate default suspension
    if (m_all_physic_info.size() == 1)
    {
        float f = 0;
        for (int i = 0; i < 4; i++)
            f += m_all_physic_info[0].m_suspension_length[i];
        m_graphical_y_offset = -f / 4 + getKartModel()->getLowestPoint();
        m_kart_model->setDefaultSuspension();
    }

}   // addReplayEvent

// ----------------------------------------------------------------------------
/** Updates the current event of the ghost kart using interpolation
 *  \param dt Time step size.
 */
void GhostKart::update(float dt)
{
    GhostController* gc = dynamic_cast<GhostController*>(getController());
    if (gc == NULL) return;

    gc->update(dt);
    if (gc->isReplayEnd())
    {
        m_node->setVisible(false);
        return;
    }

    const unsigned int idx = gc->getCurrentReplayIndex();
    const float rd         = gc->getReplayDelta();
    assert(idx < m_all_transform.size());

    float nitro_frac = 0;
    if (m_all_replay_events[idx].m_on_nitro)
    {
        nitro_frac = fabsf(m_all_physic_info[idx].m_speed) /
            (m_kart_properties->getEngineMaxSpeed());

        if (nitro_frac > 1.0f)
            nitro_frac = 1.0f;
    }
    getKartGFX()->updateNitroGraphics(nitro_frac);

    if (m_all_replay_events[idx].m_on_zipper)
        showZipperFire();

    setXYZ((1- rd)*m_all_transform[idx    ].getOrigin()
           +  rd  *m_all_transform[idx + 1].getOrigin() );

    const btQuaternion q = m_all_transform[idx].getRotation()
        .slerp(m_all_transform[idx + 1].getRotation(), rd);
    setRotation(q);

    Vec3 center_shift(0, 0, 0);
    center_shift.setY(m_graphical_y_offset);
    center_shift = getTrans().getBasis() * center_shift;

    Moveable::updateGraphics(dt, center_shift, btQuaternion(0, 0, 0, 1));
    getKartModel()->update(dt, dt*(m_all_physic_info[idx].m_speed),
        m_all_physic_info[idx].m_steer, m_all_physic_info[idx].m_speed, idx);

    Vec3 front(0, 0, getKartLength()*0.5f);
    m_xyz_front = getTrans()(front);
    getKartGFX()->update(dt);
}   // update

// ----------------------------------------------------------------------------
/** Returns the speed of the kart in meters/second. */
float GhostKart::getSpeed() const
{
    const GhostController* gc =
        dynamic_cast<const GhostController*>(getController());

    assert(gc->getCurrentReplayIndex() < m_all_physic_info.size());
    return m_all_physic_info[gc->getCurrentReplayIndex()].m_speed;
}   // getSpeed
