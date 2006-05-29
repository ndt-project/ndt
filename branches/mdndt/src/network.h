/*
 * Enhancements of NDT (Middlebox detector based on NDT)
 * Copyright (C) 2006 jeremian <jeremian [at] poczta.fm>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _JS_NETWORK_H
#define _JS_NETWORK_H

#define NDT_BACKLOG 5

#define OPT_IPV6_ONLY 1
#define OPT_IPV4_ONLY 2

int CreateListenSocket(I2Addr addr);
int CreateConnectSocket(int* sockfd, I2Addr local_addr, I2Addr server_addr);

#endif
