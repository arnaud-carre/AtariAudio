//----------------------------------------------------------
//
//	AtariAudio 1.26
//	Small & accurate ATARI-ST audio emulation
//	by Arnaud Carré aka Leonard/Oxygene (@leonard_coder)
//
//----------------------------------------------------------
#include <assert.h>
#include <memory.h>
#include "blitter.h"
#include "AtariMachine.h"


void	Blitter::Reset()
{
	memset(m_regs, 0, sizeof(m_regs));
}

uint8_t Blitter::Read8(int port)
{
	return r8(port);
}

uint16_t Blitter::Read16(int port)
{
	return r16(port);
}

void Blitter::Write8(int port, uint8_t data, AtariMachine& machine)
{
	w8(port, data);
	if (0x3c == port)
		Run(machine);
}

void Blitter::Write16(int port, uint16_t data, AtariMachine& machine)
{
	w16(port, data);
	if (0x3c == port)
		Run(machine);
}

uint16_t	Blitter::ProcessMemorySourceWord(AtariMachine& machine)
{

	uint16_t iWord;
	if (m_bNFSR && (1 == m_iXCount))			// if fxsr and we are on the last word, don't do anything
	{
		iWord = m_iCurrentMotif;
		m_iSrcAd -= m_iXSrcInc;		// HACK!!! Je pige pas bien, seule magouille que j'ai trouvé (extacy demo)
	}
	else
	{
		iWord = machine.memRead16( m_iSrcAd );
		if (m_iXCount > 1)
			m_iSrcAd += m_iXSrcInc;
	}
	return iWord;
}

uint16_t	Blitter::ReadHOP(AtariMachine& machine)
{
	uint16_t iHOP = 0;
	
	static	const	int	s_iSourceRead[ 16 ] = { 0,1,1,1,
	1,0,1,1,
	1,1,0,1,
	1,1,1,0};

	const int iLogicalHop = r8(0x3b)&0xf;
	if ( s_iSourceRead[ iLogicalHop ] )
	{
		if (r8(0x3a) & 1)
		{
			int z = 0;
		}
		switch ( r8(0x3a) & 3)
		{
			case 0:	iHOP = 0xffff;													break;
			case 1:	iHOP = r16( m_iHalfToneLine<<1 );								break;
			case 2: iHOP = ProcessMemorySourceWord(machine);								break;
			case 3: iHOP = ProcessMemorySourceWord(machine) & r16( m_iHalfToneLine<<1 );	break;
		}
	}
	return iHOP;
}

void	Blitter::InternalFetch(AtariMachine& machine)
{
	m_iCurrentMotif = (m_iCurrentMotif<<16) | ReadHOP(machine);
}

void	Blitter::Run(AtariMachine& machine)
{

	if (0 == (r8( 0x3c ) & 0x80))		// busy bit
		return;

	int iLineCount = r16( 0x38 );

	if (iLineCount > 0)
	{
		m_iCurrentMotif = 0;
		m_iXCountReset = (0 == r16( 0x36 )) ? 65536 : r16( 0x36 );

		m_iXSrcInc = int16_t(r16( 0x20 ) & (-2));
		m_iYSrcInc = int16_t(r16( 0x22 ) & (-2));
		m_iXDstInc = int16_t(r16( 0x2e ) & (-2));
		m_iYDstInc = int16_t(r16( 0x30 ) & (-2));
		const int	iShift = r8( 0x3d ) & 0x0f;
		m_bFXSR = 0 != (r8( 0x3d ) & 0x80);
		m_bNFSR = 0 != (r8( 0x3d ) & 0x40);

		const int	iLogicalOp = r8( 0x3b ) & 0x0f;

		m_iSrcAd = ((uint32_t(r16(0x24)) << 16) | r16(0x26))&0x00fffffe;
		m_iDstAd = ((uint32_t(r16(0x32)) << 16) | r16(0x34))&0x00fffffe;

		uint16_t iEndMask[ 3 ];
		iEndMask[ 0 ] = r16( 0x28 );
		iEndMask[ 1 ] = r16( 0x2a );
		iEndMask[ 2 ] = r16( 0x2c );

		m_iHalfToneLine = r8( 0x3c ) & 0x0f;
		assert( 0 == (r8(0x3c) & 0x20) );			// assume there is no smudge bit

		do
		{
			m_iXCount = m_iXCountReset;

			// first block
			if ( m_bFXSR )
			{	// FXSR
				InternalFetch(machine);
			}

			int iCurrentMaskId = 0;
			while (m_iXCount >= 1)
			{
				InternalFetch(machine);
				uint16_t iHOP = m_iCurrentMotif;
				if ( 1 != r8( 0x3a ))
					iHOP = (m_iCurrentMotif >> iShift);

				uint16_t iOriginalValue = machine.memRead16( m_iDstAd);

				uint16_t iHOPResult;
				switch (iLogicalOp)
				{
					case 0:		iHOPResult = 0;										break;
					case 1:		iHOPResult = iHOP & machine.memRead16( m_iDstAd );			break;
					case 2:		iHOPResult = iHOP & (~machine.memRead16( m_iDstAd ));		break;
					case 3:		iHOPResult = iHOP;									break;
					case 4:		iHOPResult = (~iHOP) & machine.memRead16( m_iDstAd );		break;
					case 5:		iHOPResult = machine.memRead16( m_iDstAd );				break;
					case 6:		iHOPResult = iHOP ^ machine.memRead16( m_iDstAd );			break;
					case 7:		iHOPResult = iHOP | machine.memRead16( m_iDstAd );			break;
					case 8:		iHOPResult = (~iHOP) & (~machine.memRead16( m_iDstAd ));	break;
					case 9:		iHOPResult = (~iHOP) ^ machine.memRead16( m_iDstAd );		break;
					case 10:	iHOPResult = ~machine.memRead16( m_iDstAd );				break;
					case 11:	iHOPResult = iHOP | (~machine.memRead16( m_iDstAd ));		break;
					case 12:	iHOPResult = ~iHOP;									break;
					case 13:	iHOPResult = (~iHOP) | machine.memRead16( m_iDstAd );		break;
					case 14:	iHOPResult = (~iHOP) | (~machine.memRead16( m_iDstAd ));	break;
					case 15:	iHOPResult = 0xffff;								break;
				}

				iHOPResult = (iHOPResult & iEndMask[ iCurrentMaskId ]) | (iOriginalValue & (~iEndMask[ iCurrentMaskId ]));

				machine.memWrite16( m_iDstAd, iHOPResult );

				if (m_iXCount > 1)
					m_iDstAd += m_iXDstInc;

				m_iXCount--;
				if (0 == iCurrentMaskId)
					iCurrentMaskId = 1;		// middle mask
				if (1 == m_iXCount)
					iCurrentMaskId = 2;		// end mask
			}

			// end of line
			m_iSrcAd += m_iYSrcInc;
			m_iDstAd += m_iYDstInc;
			if (m_iYDstInc >= 0)
				m_iHalfToneLine = (m_iHalfToneLine + 1) & 15;
			else
				m_iHalfToneLine = (m_iHalfToneLine - 1) & 15;
		}
		while (--iLineCount);

		w16(0x24, m_iSrcAd >> 16);
		w16(0x26, m_iSrcAd&0xfffe);
		w16(0x32, m_iDstAd >> 16);
		w16(0x34, m_iDstAd&0xfffe);
		w16(0x38, 0);
	}

	m_regs[0x3c] &= 0x7f;
}
