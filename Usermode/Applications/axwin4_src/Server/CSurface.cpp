/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CWindow.cpp
 * - Window
 */
#include <CSurface.hpp>
#include <cassert>
#include <stdexcept>
#include <cstring>
#include <system_error>
#include <cerrno>

namespace AxWin {

CSurface::CSurface(int x, int y, unsigned int w, unsigned int h):
	m_rect(x,y, w,h),
	m_fd(-1),
	m_data(0)
{
	if( w > 0 && h > 0 )
	{
		m_data = new uint32_t[w * h];
	}
}

CSurface::~CSurface()
{
}

uint64_t CSurface::GetSHMHandle()
{
	// 1. Free local buffer
	delete m_data;
	// 2. Allocate a copy in SHM
	m_fd = _SysOpen("/Devices/shm/anon", OPENFLAG_WRITE|OPENFLAG_READ);
	if(m_fd==-1)	throw ::std::system_error(errno, ::std::system_category());
	size_t	size = m_rect.m_w*m_rect.m_h*4;
	_SysTruncate(m_fd, size);
	// 3. mmap shm copy
	m_data = static_cast<uint32_t*>( _SysMMap(nullptr, size, MMAP_PROT_WRITE, 0, m_fd, 0) );
	if(!m_data)	throw ::std::system_error(errno, ::std::system_category());
	return _SysMarshalFD(m_fd);
}

void CSurface::Resize(unsigned int W, unsigned int H)
{
	if( m_fd == -1 )
	{
		// Easy realloc
		// TODO: Should I maintain window contents sanely? NOPE!
		delete m_data;
		m_data = new uint32_t[W * H];
	}
	else
	{
		//_SysIOCtl(m_fd, SHM_IOCTL_SETSIZE, W*H*4);
	}
	m_rect.Resize(W, H);
}

void CSurface::DrawScanline(unsigned int row, unsigned int x_ofs, unsigned int w, const void* data)
{
	if( row >= m_rect.m_h )
		throw ::std::out_of_range("CSurface::DrawScanline row");
	if( x_ofs >= m_rect.m_w )
		throw ::std::out_of_range("CSurface::DrawScanline x_ofs");

	if( w > m_rect.m_w )
		throw ::std::out_of_range("CSurface::DrawScanline width");
	
	size_t	ofs = row*m_rect.m_w + x_ofs;
	::memcpy( &m_data[ofs], data, w*4 );
}

const uint32_t* CSurface::GetScanline(unsigned int row, unsigned int x_ofs) const
{
	if( row >= m_rect.m_h )
		throw ::std::out_of_range("CSurface::GetScanline row");
	if( x_ofs >= m_rect.m_w )
		throw ::std::out_of_range("CSurface::GetScanline x_ofs");

	return &m_data[row * m_rect.m_w + x_ofs];
}


};	// namespace AxWin


