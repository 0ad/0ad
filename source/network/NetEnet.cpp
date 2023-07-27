/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "NetEnet.h"

#include "ps/ConfigDB.h"
#include "ps/containers/Span.h"

namespace PS
{

namespace Enet
{

// ENet protocol MTU
// ENet by default uses 1400 as ENet packet fragment max size, adding ICMP and
// IPv4 headers this may exceed the MTU for some VPN solutions [1], so provide
// a lower default.
// MTU negotiation server-side needs [2], which was merged after enet-1.3.17,
// so the user configured value may be ignored on older versions.
// [1] https://github.com/lsalzman/enet/issues/132
// [2] https://github.com/lsalzman/enet/pull/222
constexpr enet_uint32 HOST_DEFAULT_MTU = 1372;

ENetHost* CreateHost(const ENetAddress* address, size_t peerCount, size_t channelLimit)
{
	// TODO: Maybe allow user to set rate limits?

	ENetHost* host = enet_host_create(address, peerCount, channelLimit, 0, 0);
	if (!host)
		return nullptr;

	// Public ENet API doesn't offer a means to change MTU, so do it in a
	// way least likely to break with ENet updates.
	enet_uint32 mtu = HOST_DEFAULT_MTU;
	CFG_GET_VAL("network.enetmtu", mtu);
	host->mtu = mtu;
	for (ENetPeer& p : PS::span{host->peers, host->peerCount})
		enet_peer_reset(&p);

	return host;
}

} // namespace Enet

} // namespace PS
