/*
 * This Arduino library is for reading buttons
 * Copyright (C) 2016  Pierre-Loup Martin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This library is for reading buttons.
 * Simply create a new instance of the library in your sketch.
 * Call begin() and pass the number of the pin the button is attached to.
 * Call update() as often has possible, then use the different methods provided to know switch state.
 */

/*
 * Once you have several buttons created, you may want to update all their readings at once:
 * Create one (or several) instances of PushButtonGroup, and add the button you've created to it.
 * Once done, you can call an update for each group of buttons.
 * The buttons attached to the group can be added or removed at any moment, as they are dynamicly attached.
 * Remove a button from a group doesn't suppress it, it will still be usable alone and/or in other groups.
 */

 /* WARNING: this class uses dynamic memory allocation.
  * Compiler won't warn you about using too much memory size.
  * It's up to you to know that you don't overflow what can be used.
  * Each button set occupies 19 bytes of memory, and the ButtonGroup is 2 bytes per button, + 2.
  * When adding or removing a button to buttonGroup, you should have enough continguous memory to store a temp allocation of the table.
  */


#include "PushButton.h"

//this creates an instance of a push button.
PushButton::PushButton(){

	_debounceDelay = 5;
	_longDelay = 1000;
	_doubleDelay = 300;

	_time = millis();

}

//Initialize a button with the given pin, and sets up initial state
void PushButton::begin(int pin, byte mode){

	_pin = pin;
	_pinMode = mode;

	if(_pinMode == INPUT_PULLUP){
		pinMode(_pin, INPUT_PULLUP);
	} else {
		pinMode(_pin, INPUT);
	}

	if(_pinMode == INPUT_PULLUP || _pinMode == PULLUP){
		_invert = true;

		_state = true;
		_pState = true;
		_now = true;
		_prev = true;
	} else {
		_invert = false;

		_state = false;
		_pState = false;
		_now = false;
		_prev = false;
	}

	_longState = false;
	_longClick = false;

	_isJustPressed = true;
	_isJustReleased = true;
}

//Set debounce delay
void PushButton::setDebounceDelay(int delay){
	_debounceDelay = delay;
}

//Set delay for long clicks and releases.
void PushButton::setLongDelay(int delay){
	_longDelay = delay;
}

//Set delay for double clicks.
void PushButton::setDoubleDelay(int delay){
	_doubleDelay = delay;
}


//Function that update reading. To be called as often as possible, typically at the beginning of each loop.
bool PushButton::update(){

	//Store the previous instant state, read the new one.
	_prev = _now;
	_now = digitalRead(_pin);

	//If different, set timer to zero
	if(_now != _prev){
		_time = millis();
		return false;
	}

	//If instant state is different than button state, and time elapsed greater thant debounce time, change state.
	if((_state != _now) && ((millis() - _time) > _debounceDelay)){
		_pState = _state;
		_state = _now;
		_longState = false;
		_isJustPressed = false;
		_isJustReleased = false;

		if(isPressed()){
			_longClick = false;
		}

		if(isReleased()){
			if((millis() - _timeDouble) < _doubleDelay){
				_doubleClick = true;
			} else {
				_doubleClick = false;
			}
			_timeDouble = millis();
		}
		return true;
	}

	//If time elapsed since changed is greater than delay for long presses, set longState.
	if(!_longState && ((millis() - _time) > _longDelay)){
		_longState = true;
		if(isPressed()){
			_longClick = true;
		}
		return true;
	}

	return false;
}

//Return true if button is pressed.
bool PushButton::isPressed(){
	return _state ^ _invert;
}

//Return true if button is released.
bool PushButton::isReleased(){
	return !_state ^ _invert;
}

//Return true when the button is pressed for a long time.
bool PushButton::isLongPressed(){
	return (_state ^ _invert) && _longState;
}

//Return true if button is released for a long time
bool PushButton::isLongReleased(){
	return (!_state ^ _invert) && _longState;
}

//Return true when button is newly pressed.
//Returns true only once, then it needs to be released to return true again.
bool PushButton::justPressed(){
	if(isPressed() && !_isJustPressed){
		_isJustPressed = true;
		return true;
	} else {
		return false;
	}
}

//Return true when button is newly released.
//Returns true only once, then is needs to be pressed to return true again.
//click methods are based on it, so don't use it if you want to call justClicked(), justLongClicked() or 
//justDoubleClick().
bool PushButton::justReleased(){
	if(isReleased() && !_isJustReleased){
		_isJustReleased = true;
		return true;
	} else {
		return false;
	}
}

//Return true if a click has happened, that is not a long click, or the first of a double click.
//This function will return false if a click has happenned for a time lesser than doubleDelay.
//If you need to immediately know a clicked has been made, use justReleased() instead.
bool PushButton::justClicked(){
	if(((millis() - _timeDouble) < _doubleDelay) || (_longClick || _doubleClick)) return false;

	return justReleased();
}

//Return true if a long click has happened.
bool PushButton::justLongClicked(){
	if(!_longClick) return false;

	if(justReleased()){
		_longClick = false;
		return true;
	} else {
		return false;
	}
}

//return true if a double click has happened.
bool PushButton::justDoubleClicked(){
	if(!_doubleClick) return false;

	if(justReleased()){
		_doubleClick = false;
		return true;
	} else {
		return false;
	}
}


//Create a button group.
PushButtonGroup::PushButtonGroup(){
	buttons = NULL;
	buttonsSize = 0;
}

//Delete a button group.
PushButtonGroup::~PushButtonGroup(){
	free(buttons);
}

//Add a new button to the group. Use ralloc to dynamicly change the size of the pointers table
void PushButtonGroup::add(PushButton* button){
	buttonsSize++;
	PushButton **tmp = (PushButton**)realloc(buttons, sizeof(PushButton) * buttonsSize);
	if(tmp == NULL){
		return;
	}
	buttons = tmp;
	buttons[buttonsSize - 1] = button;
}

//remove a button from the group.
void PushButtonGroup::remove(PushButton* button){
	buttonsSize--;
	if(buttonsSize == 0){
		delete(buttons);
		return;
	}

	PushButton **tmp = (PushButton**)realloc(buttons, sizeof(PushButton) * buttonsSize);
	if(tmp == NULL){
		return;
	}
	
	int top = buttonsSize + 1;
	int count = 0;

	for(int i = 0; i < top; i++){
		if(buttons[i] == button){continue;}
		tmp[count] = buttons[i];
		count++;
	}

	buttons = tmp;
}

//Update the readings of all buttons.
bool PushButtonGroup::update(){
	bool state = false;
	for(int i = 0; i < buttonsSize; i++){
		if(buttons[i]->update()){
			state = true;
/*
			Serial.print("button ");
			Serial.print(i);
			Serial.print(": ");
			Serial.print(buttons[i]->isPressed());
			Serial.println();
			Serial.println();
*/
		}
	}
	return state;
}