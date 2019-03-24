#ifndef _XENODON_MAIN_LOOP_H
#define _XENODON_MAIN_LOOP_H

struct EventDispatcher;
struct Display;
class Logger;

void main_loop(EventDispatcher& dispatcher, Display* display);

#endif
