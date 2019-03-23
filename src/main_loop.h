#ifndef _XENODON_MAIN_LOOP_H
#define _XENODON_MAIN_LOOP_H

struct EventDispatcher;
struct Display;
class Logger;

void main_loop(Logger& logger, EventDispatcher& dispatcher, Display* display);

#endif
