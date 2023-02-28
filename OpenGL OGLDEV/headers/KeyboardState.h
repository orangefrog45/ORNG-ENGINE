#pragma once
struct KeyboardState {
	bool wPressed;
	bool aPressed;
	bool sPressed;
	bool dPressed;
	bool ePressed;
	bool qPressed;
	bool shiftPressed;
	bool ctrlPressed;
	bool ESC_pressed;
	bool mouse_locked;
	KeyboardState();
	void SetMouseLocked(bool t) { mouse_locked = t; };
	void KeyboardCB(unsigned char key, int mouse_x, int mouse_y);
	void KeysUp(unsigned char key, int x, int y);
	void SpecialKeyboardCB(int key, int mouse_x, int mouse_y);
	void SpecialKeysUp(int key, int x, int y);

};

