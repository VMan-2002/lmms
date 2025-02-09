/*
 * vibrating_sring.h - model of a vibrating string lifted from pluckedSynth
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
 * 
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */
#include <cmath>

#include "VibratingString.h"
#include "interpolation.h"
#include "AudioEngine.h"
#include "Engine.h"


VibratingString::VibratingString(	float _pitch, 
					float _pick,
					float _pickup,
					float * _impulse, 
					int _len,
					sample_rate_t _sample_rate,
					int _oversample,
					float _randomize,
					float _string_loss,
					float _detune,
					bool _state ) :
	m_oversample( 2 * _oversample / (int)( _sample_rate /
				Engine::audioEngine()->baseSampleRate() ) ),
	m_randomize( _randomize ),
	m_stringLoss( 1.0f - _string_loss ),
	m_state( 0.1f )
{
	m_outsamp = new sample_t[m_oversample];
	int string_length;
	
	string_length = static_cast<int>( m_oversample * _sample_rate /
								_pitch ) + 1;
	string_length += static_cast<int>( string_length * -_detune );

	int pick = static_cast<int>( ceil( string_length * _pick ) );
	
	if( ! _state )
	{
		m_impulse = new float[string_length];
		resample( _impulse, _len, string_length );
	}
	else
 	{
		m_impulse = new float[_len];
		for( int i = 0; i < _len; i++ )
		{
			m_impulse[i] = _impulse[i];
		}
	}
	
	m_toBridge = VibratingString::initDelayLine( string_length, pick );
	m_fromBridge = VibratingString::initDelayLine( string_length, pick );

	
	VibratingString::setDelayLine( m_toBridge, pick, 
						m_impulse, _len, 0.5f, 
						_state );
	VibratingString::setDelayLine( m_fromBridge, pick, 
						m_impulse, _len, 0.5f,
						_state);
	
	m_choice = static_cast<int>( m_oversample * 
				static_cast<float>( rand() ) / RAND_MAX ); 
	
	m_pickupLoc = static_cast<int>( _pickup * string_length );
}




VibratingString::delayLine * VibratingString::initDelayLine( int _len,
								int _pick )
{
	delayLine * dl = new VibratingString::delayLine[_len];
	dl->length = _len;
	if( _len > 0 )
	{
		dl->data = new sample_t[_len];
		float r;
		float offset = 0.0f;
		for( int i = 0; i < dl->length; i++ )
		{
			r = static_cast<float>( rand() ) /
					RAND_MAX;
			offset =  ( m_randomize / 2.0f -
					m_randomize ) * r;
			dl->data[i] = offset;
		}
	}
	else
	{
		dl->data = nullptr;
	}

	dl->pointer = dl->data;
	dl->end = dl->data + _len - 1;

	return( dl );
}




void VibratingString::freeDelayLine( delayLine * _dl )
{
	if( _dl )
	{
		delete[] _dl->data;
		delete[] _dl;
	}
}




void VibratingString::resample( float *_src, f_cnt_t _src_frames,
							 f_cnt_t _dst_frames )
{
	for( f_cnt_t frame = 0; frame < _dst_frames; ++frame )
	{
		const float src_frame_float = frame * 
						(float) _src_frames / 
							_dst_frames;
		const float frac_pos = src_frame_float -
				static_cast<f_cnt_t>( src_frame_float );
		const f_cnt_t src_frame = qBound<f_cnt_t>(
				1, static_cast<f_cnt_t>( src_frame_float ),
							_src_frames - 3 );
		m_impulse[frame] = cubicInterpolate(
						_src[src_frame - 1],
						_src[src_frame + 0],
						_src[src_frame + 1],
						_src[src_frame + 2],
						frac_pos );
	}
}

