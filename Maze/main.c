//
//  main.c
//  New 3pi
//
//  Created by Justin Nguyen on 1/20/18.
//  Copyright   2018 Justin Nguyen. All rights reserved.
//

#include <pololu/3pi.h>
#include <avr/pgmspace.h>

const char welcome_line1[] PROGMEM = " Pololu";
const char welcome_line2[] PROGMEM = "3\xf7 Robot";
const char demo_name_line1[] PROGMEM = "Line";
const char demo_name_line2[] PROGMEM = "follower";

// A couple of simple tunes, stored in program space.
const char welcome[] PROGMEM = ">g32>>c32";
const char go[] PROGMEM = "L16 cdegreg4";

const char levels[] PROGMEM = {
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111,
	0b11111
};


void load_custom_characters()
{
	lcd_load_custom_character(levels+0,0); // no offset, e.g. one bar
	lcd_load_custom_character(levels+1,1); // two bars
	lcd_load_custom_character(levels+2,2); // etc...
	lcd_load_custom_character(levels+3,3);
	lcd_load_custom_character(levels+4,4);
	lcd_load_custom_character(levels+5,5);
	lcd_load_custom_character(levels+6,6);
	clear(); // the LCD must be cleared for the characters to take effect
}

// This function displays the sensor readings using a bar graph.
void display_readings(const unsigned int *calibrated_values)
{
	unsigned char i;
	
	for(i=0;i<5;i++) {
		
		const char display_characters[10] = {' ',0,0,1,2,3,4,5,6,255};
		
		
		char c = display_characters[calibrated_values[i]/101];
		
		// Display the bar graph character.
		print_character(c);
	}
}

// Initializes the 3pi, displays a welcome message, calibrates, and
// plays the initial music.
void initialize()
{
	unsigned int counter; // used as a simple timer
	unsigned int sensors[5]; // an array to hold sensor values
	
	pololu_3pi_init(2000);
	load_custom_characters(); // load the custom characters
	
	// Play welcome music and display a message
	print_from_program_space(welcome_line1);
	lcd_goto_xy(0,1);
	print_from_program_space(welcome_line2);
	play_from_program_space(welcome);
	delay_ms(1000);
	
	clear();
	print_from_program_space(demo_name_line1);
	lcd_goto_xy(0,1);
	print_from_program_space(demo_name_line2);
	delay_ms(1000);
	
	// Display battery voltage and wait for button press
	while(!button_is_pressed(BUTTON_B))
	{
		int bat = read_battery_millivolts();
		
		clear();
		print_long(bat);
		print("mV");
		lcd_goto_xy(0,1);
		print("Press B");
		
		delay_ms(100);
	}
	
	// Always wait for the button to be released so that 3pi doesn't
	// start moving until your hand is away from it.
	wait_for_button_release(BUTTON_B);
	delay_ms(1000);
	
	// Auto-calibration: turn right and left while calibrating the
	// sensors.
	for(counter=0;counter<80;counter++)
	{
		if(counter < 20 || counter >= 60)
		set_motors(40,-40);
		else
		set_motors(-40,40);
		
		
		calibrate_line_sensors(IR_EMITTERS_ON);
		
		// Since our counter runs to 80, the total delay will be
		// 80*20 = 1600 ms.
		delay_ms(20);
	}
	set_motors(0,0);
	
	// Display calibrated values as a bar graph.
	while(!button_is_pressed(BUTTON_B))
	{
		// Read the sensor values and get the position measurement.
		unsigned int position = read_line(sensors,IR_EMITTERS_ON);
		
		clear();
		print_long(position);
		lcd_goto_xy(0,1);
		display_readings(sensors);
		
		delay_ms(100);
	}
	wait_for_button_release(BUTTON_B);
	
	clear();
	
	print("Go!");
	
	// Play music and wait for it to finish before we start driving.
	play_from_program_space(go);
	while(is_playing());
}

char path[100] = ""; 
unsigned char path_length = 0;

void turn(char dir) {
 switch(dir) { 
	 case 'L': 
		set_motors(-80,80); 
		delay_ms(200); 
		break; 
		case 'R': 
			set_motors(80,-80); 
			delay_ms(200); 
			break; 
		case 'B': 
			set_motors(80,-80); 
			delay_ms(400); 
			break; 
		case 'S':
			break; 
		}
 }


char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right) { 
	if(found_left) 
		return 'L'; 
	else if(found_straight) 
		return 'S'; 
	else if(found_right) 
		return 'R'; 
	else 
		return 'B'; 
} 

void follow_segment() { 
	int last_proportional = 0;
	long integral=0; 
	while(1) {  
		unsigned int sensors[5];
		unsigned int position = read_line(sensors,IR_EMITTERS_ON);

		int proportional = ((int)position) - 2000; 
		int derivative = proportional - last_proportional; integral += proportional; 
		last_proportional = proportional; 
		int power_difference = proportional/20 + integral/10000 + derivative*3/2; 
		const int max = 60; 
		if(power_difference > max) 
			power_difference = max; 
		if(power_difference < -max) 
			power_difference = -max; 
		if(power_difference < 0) 
			set_motors(max+power_difference,max); 
		else 
			set_motors(max,max-power_difference); 
			
		if(sensors[1] < 100 && sensors[2] < 100 && sensors[3] < 100) { 
			 return; 
		} else if(sensors[0] > 200 || sensors[4] > 200) {  
			return; 
		}
}
}

int main()
{
	
	
	 initialize();

	while(1)
	{
		follow_segment();
		
		set_motors(50,50);
		delay_ms(50);
		
		unsigned char found_left=0; 
		unsigned char found_straight=0;
		unsigned char found_right=0; 
		
		unsigned int sensors[5];
		read_line(sensors,IR_EMITTERS_ON);
		if(sensors[0] > 100) 
			found_left = 1; 
		if(sensors[4] > 100) 
			found_right = 1; 
		set_motors(40,40); 
		delay_ms(200);
		read_line(sensors,IR_EMITTERS_ON); 
		if(sensors[1] > 200 || sensors[2] > 200 || sensors[3] > 200) 
			found_straight = 1;
		if(sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600) 
			break;  
		unsigned char dir = select_turn(found_left, found_straight, found_right); 
		turn(dir);
		path[path_length] = dir; 
		path_length ++; 

	}
}
