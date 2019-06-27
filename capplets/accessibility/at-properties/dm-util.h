/* vi: set sw=4 ts=4 wrap ai: */
/*
 * dm-util.h: This file is part of mate-control-center.
 *
 * Copyright (C) 2019 Wu Xiaotian <yetist@gmail.com>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * */

#ifndef __DM_UTIL_H__
#define __DM_UTIL_H__  1

G_BEGIN_DECLS

typedef enum
{
    DM_TYPE_UNKNOWN,
    DM_TYPE_LIGHTDM,
    DM_TYPE_GDM,
    DM_TYPE_MDM,
} DMType;

DMType dm_get_type(void);

G_END_DECLS

#endif /* __DM_UTIL_H__ */
