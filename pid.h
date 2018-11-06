/* Copyright (c) 2018 Frans Korhonen, Institute for Atmospheric and Earth System Research / Physics, Faculty of Science, University of Helsinki, Finland

This file is part of RMCIOS.

RMCIOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RMCIOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RMCIOS.  If not, see <http://www.gnu.org/licenses/>.
*/

/* 2015 Frans Korhonen. University of Helsinki.
 * PID control function
 * 2017-11-17 FK, Added conditional for P-only loop to prevent division by zero
 * 2018-01-23 FK, Added safe-output variable to set output to safe value on calculation error (-Inf +Inf NaN etc)
 */
#ifndef pid_h
#define pid_h

struct pid_type
{
	float setpoint ;
	float Kp ;
	float Ki ;
	float Kd ;
	
	float input_min ;
	float input_max ;
	
	float output_min ;
	float output_max ;

	float previous_error ;
	float integral ;
	float output ;

	float safe_output ;

	// Channel interface added:
	int linked_channel ;
	int clock_channel ;
} ;

float pid_control(struct pid_type *t,float measured_value,float dt)
{
	// force measured value between max input range:
	if(measured_value > t->input_max  ) measured_value=t->input_max ;
	else if(measured_value < t->input_min ) measured_value=t->input_min ;

	// Calculate pid:
	float error = t->setpoint - measured_value ;
  	float derivative= (error - t->previous_error ) ;
	t->integral = t->integral + error*dt ;
	derivative = (error - t->previous_error)/dt ;
  	t->output = t->Kp*error + t->Ki*t->integral + t->Kd*derivative ;
  	t->previous_error = error ;
	
	// Force integral to fit output inside output value limits:
	if( t->output > t->output_max && t->Ki!=0)
	{
		t->previous_error=0; // ignore the derivative
		t->integral= (t->output_max - t->Kp * error)/t->Ki ;
  		t->output = t->Kp*error + t->Ki*t->integral ;
	}
	else if( t->output < t->output_min && t->Ki!=0)
	{
		t->previous_error=0; // ignore the derivative
		t->integral= (t->output_min - t->Kp * error)/t->Ki ;
  		t->output = t->Kp*error + t->Ki*t->integral ;
	}

	if( !isnormal( t->output) )
	{
		t->output=t->safe_output ;
		t->integral=0 ;
		t->previous_error=0 ;
	}
				
	return t->output ;
}

#endif 



