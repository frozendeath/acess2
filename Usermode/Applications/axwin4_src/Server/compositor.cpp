/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * compositor.cpp
 * - Window compositor
 */
#include <video.hpp>
#include <compositor.hpp>
#include <CCompositor.hpp>

namespace AxWin {

CCompositor::CCompositor(CVideo& video):
	m_video(video)
{
	// 
}

CWindow* CCompositor::CreateWindow(CClient& client)
{
	return new CWindow(client, "TODO");
}

void CCompositor::Redraw()
{
	// Redraw the screen and clear damage rects
	if( m_damageRects.empty() )
		return ;
	
	// Build up foreground grid (Rects and windows)
	// - This should already be built (mutated on window move/resize/reorder)
	
	// For all windows, check for intersection with damage rects
	for( auto rect : m_damageRects )
	{
		for( auto window : m_windows )
		{
			if( rect.HasIntersection( window->m_surface.m_rect ) )
			{
				// TODO: just reblit
				CRect	rel_rect = window->m_surface.m_rect.RelativeIntersection(rect);
				BlitFromSurface( window->m_surface, rel_rect );
				//window->Repaint( rel_rect );
			}
		}
	}

	m_damageRects.clear();
}

void CCompositor::DamageArea(const CRect& area)
{
	// 1. Locate intersection with any existing damaged areas
	// 2. Append after removing intersections
}

void CCompositor::BlitFromSurface(const CSurface& dest, const CRect& src_rect)
{
	for( unsigned int i = 0; i < src_rect.m_h; i ++ )
	{
		m_video.BlitLine(
			dest.GetScanline(src_rect.m_y, src_rect.m_y),
			dest.m_rect.m_y + src_rect.m_y + i,
			dest.m_rect.m_x + src_rect.m_x,
			src_rect.m_w
			);
	}
}

}	// namespace AxWin

