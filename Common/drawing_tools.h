// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

inline bool has_overlap(const RECT& rect0, const RECT& rect1)
{
	if (rect0.right < rect1.left) return false;
	if (rect1.right < rect0.left) return false;
	if (rect0.bottom < rect1.top) return false;
	if (rect1.bottom < rect0.top) return false;
	return true;
}

inline bool is_inside(const POINT& pt, const RECT& rect)
{
	if (pt.x < rect.left) return false;
	if (pt.x > rect.right) return false;
	if (pt.y < rect.top) return false;
	if (pt.y > rect.bottom) return false;
	return true;
}

inline int width(const RECT& rect)
{
	return rect.right - rect.left;
}

inline int height(const RECT& rect)
{
	return rect.bottom - rect.top;
}