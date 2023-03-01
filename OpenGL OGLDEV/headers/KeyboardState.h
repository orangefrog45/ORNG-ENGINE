#pragma once
struct KeyboardState {
	bool wPressed = false;
	bool aPressed = false;
	bool sPressed = false;
	bool dPressed = false;
	bool ePressed = false;
	bool qPressed = false;
	bool g_pressed = false;
	bool shiftPressed = false;
	bool ctrlPressed = false;
	bool ESC_pressed = false;
	bool mouse_locked = false;
	KeyboardState();
	void SetMouseLocked(bool t) { mouse_locked = t; };
	void KeyboardCB(unsigned char key, int mouse_x, int mouse_y);
	void KeysUp(unsigned char key, int x, int y);
	void SpecialKeyboardCB(int key, int mouse_x, int mouse_y);
	void SpecialKeysUp(int key, int x, int y);

};

