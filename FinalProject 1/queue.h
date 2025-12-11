/*
 * queue.h
 *
 *  Created on: Nov 23, 2025
 *      Author: Dallas.Owens
 */

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct {
    char direction;           // F, B, R, L
    unsigned int duration;    // Time units
    unsigned char valid;      // Was parse successful?
} ParsedCommand;



#endif /* QUEUE_H_ */
