#ifdef _HARMATTAN
#include "vkb.h"

bool karinAddActionPollEvent(unsigned action, bool state)
{
	return true;
}
bool karinAddTouchPollEvent(unsigned action, int d)
{
	return true;
}
unsigned karinPollActionInputEvents(void)
{
	return 0;
}
unsigned karinPollTouchInputEvents(void)
{
	return 0;
}
bool karinReturnActionInputEvent(const int n, unsigned &action, bool &state)
{
	return false;
}
bool karinReturnTouchInputEvent(const int n, unsigned &action, int &value)
{
	return false;
}
void karinEndActionInputEvents(void)
{
}
void karinEndTouchInputEvents(void)
{
}

void karinSetClientState(unsigned state)
{
}

#endif

