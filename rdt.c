#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// **************************************************************************************
// ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
// FROM VERSION 1.1 of J.F.Kurose
//
//  This code should be used for PA2, unidirectional or bidirectional
//  data transfer protocols (from A to B. Bidirectional transfer of data
//  is for extra credit and is not required). Network properties:
//      - one way network delay averages five time units (longer if there
//          are other messages in the channel for GBN), but can be larger
//      - packets can be corrupted (either the header or the data portion)
//          or lost, according to user-defined probabilities
//      - packets will be delivered in the order in which they were sent
//          (although some can be lost).
// **************************************************************************************

#define BIDIRECTIONAL 0 // Change to 1 if you're doing extra credit and write a routine called B_output

// *************************** GLOBAL CONSTANTS AND VARIABLES ***************************

/** For my debug purposes. Any non-zero value will take debug inputs and print outputs. */
const int kDebugMode = 1;

/** Name of input file in debug mode. */
const char kDebugInputFile[25] = "debug_input.txt";

/** Size of data array in this simulation. */
const int kDataSize = 20;

/** Character to be sent in the payload of an acknowledgement packet (6 is the ASCII character for acknowledgement). */
const char kAckChar = 6;

/** Character to be sent in the payload of an non-acknowledgement packet (21 is the ASCII character for negative acknowledgement). */
const char kNackChar = 21;

/** Sequence number for A. Used by A_output. Initially set to 1. */
int g_seqnum_of_a = 1;

/** Sequence number for B. Used by B_input. Initially set to 0—when no packet has arrived. */
int g_seqnum_of_b = 0;

/**
 * A "msg" is the data unit passed from layer 5 (teachers code) to layer 4 (students' code). It contains the data (characters) to be delivered to layer 5 via the students transport level protocol entities.
 */
struct msg {
	char data[20];
};

/**
 * A packet is the data unit passed from layer 4 (students code) to layer 3 (teachers code). Note the pre-defined packet structure, which all students must follow.
 */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

// ****************** FUNCTION PROTOTYPES. DEFINED IN THE LATER PART. ******************

void starttimer (int AorB, float increment);

void stoptimer (int AorB);

void tolayer3 (int AorB, struct pkt packet);

void tolayer5 (int AorB, char datasent[20]);

int CalculateCheckSum (struct pkt packet);

void ReadInputFile ();

// *********************** STUDENTS WRITE THE NEXT SEVEN ROUTINES ***********************

/**
 * Called from layer 5, passed the data to be sent to other side.
 * @param message A structure of type msg, containing data to be sent to the B-side.
 */
void A_output (struct msg message) {
	struct pkt packet;

	packet.seqnum = g_seqnum_of_a++;    // Sets current seqnum and increases the global value.
	packet.acknum = 0;                  // Zero when NOT acknowledged.
	for (int i = 0; i<kDataSize; i++) packet.payload[i] = message.data[i];
	packet.checksum = CalculateCheckSum(packet);

	tolayer3(0, packet);
}

/**
 * Need be completed only for extra credit. Note that with simplex transfer from a-to-B, there is no B_output().
 * @param message
 */
void B_output (struct msg message) {

}

/**
 * Called from layer 3, when a packet arrives for layer 4.
 * @param packet
 */
void A_input (struct pkt packet) {
	if (packet.acknum==1) {
		{
			// TODO: Remove this debug block.
			printf("A packet has been acknowledged. Yey!\n");
			printf("%d, %d, %d\n", packet.seqnum, packet.acknum, packet.checksum);
			for (int i = 0; i<kDataSize; i++) printf("%c", packet.payload[i]);
			printf("\n");
		}
	}
	else {
		{
			// TODO: Remove this debug block.
			printf("Packet %d has NOT been acknowledged.\n", packet.seqnum);
		}
	}
}

/**
 * Called when A's timer goes off.
 */
void A_timerinterrupt (void) {

}

/**
 * The following routine will be called once (only) before any other entity A routines are called. You can use it to do any initialization.
 */
void A_init (void) {

}

/**
 * Called from layer 3, when a packet arrives for layer 4 at B.
 * @param packet The packet which arrived.
 */
void B_input (struct pkt packet) {
	// Create a packet to send as ack or nack.
	struct pkt ack_or_nack_packet;
	ack_or_nack_packet.seqnum = packet.seqnum;

	// Check corruption.
	if (packet.checksum==CalculateCheckSum(packet)) {
		// Packet is NOT corrupted. Check sequence.
		if (packet.seqnum==g_seqnum_of_b + 1) {
			// Sequence is also okay. Send acknowledgement.
			ack_or_nack_packet.acknum = 1; // 1 when acknowledged.
			for (int i = 0; i<kDataSize; i++) ack_or_nack_packet.payload[i] = kAckChar;
			ack_or_nack_packet.checksum = CalculateCheckSum(ack_or_nack_packet);
			tolayer3(1, ack_or_nack_packet);

			// And send the data to layer 5.
			struct msg message;
			for (int i = 0; i<kDataSize; i++) message.data[i] = packet.payload[i];
			tolayer5(1, message.data);
		}
		else {
			// Packet is okay, but the sequence is not.
			// TODO: Implement this case.
		}
	}
	else {
		// Packet got corrupted. Send non-acknowledgement.
		ack_or_nack_packet.acknum = 0; // 0 when not acknowledged.
		for (int i = 0; i<kDataSize; i++) ack_or_nack_packet.payload[i] = kNackChar;
		ack_or_nack_packet.checksum = CalculateCheckSum(ack_or_nack_packet);
		tolayer3(1, ack_or_nack_packet);
	}
}

/**
 * Called when B's timer goes off.
 */
void B_timerinterrupt (void) {
	printf(" B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/**
 * The following routine will be called once (only) before any other entity B routines are called. You can use it to do any initialization.
 */
void B_init (void) {

}

// ************************ NETWORK EMULATION CODE STARTS BELOW *************************
// The code below emulates the layer 3 and below network environment:
//      - emulates the transmission and delivery (possibly with bit-level corruption
//          and packet loss) of packets across the layer 3/4 interface
//      - handles the starting/stopping of a timer, and generates timer
//          interrupts (resulting in calling students timer handler).
//      - generates message to be sent (passed from later 5 to 4)
//
// THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
// THE CODE BELOW. YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
// OF THE DATA STRUCTURES BELOW. If you're interested in how I designed
// the emulator, you're welcome to look at the code - but again, you should have
// to, and you definitely should not have to modify
// **************************************************************************************

struct event {
	float evtime;       // Event time
	int evtype;         // Event type code
	int eventity;       // Entity where event occurs
	struct pkt *pktptr; // ptr to packet (if any) assoc w/ this event
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL; // The event list

/* Possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     // For my debugging
int nsim = 0;      // Number of messages from 5 to 4 so far
int nsimmax = 0;   // Number of msgs to generate, then stop
float time = 0.000;
float lossprob;    // Probability that a packet is dropped
float corruptprob; // Probability that one bit is packet is flipped
float lambda;      // Arrival rate of messages from layer 5
int ntolayer3;     // Number sent into layer 3
int nlost;         // Number lost in media
int ncorrupt;      // Number corrupted by media

void init ();

void generate_next_arrival (void);

void insertevent (struct event *p);

int main () {
	struct event *eventptr;
	struct msg msg2give;
	struct pkt pkt2give;

	int i, j;
	char c;

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist; // Get next event to simulate
		if (eventptr==NULL)
			goto terminate;
		evlist = evlist->next; // Remove this event from event list
		if (evlist!=NULL)
			evlist->prev = NULL;
		if (TRACE>=2) {
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype==0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype==1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime; // Update time to next event time
		if (eventptr->evtype==FROM_LAYER5) {
			if (nsim<nsimmax) {
				if (nsim + 1<nsimmax)
					generate_next_arrival(); // Set up future arrival fill in msg to give with string of same letter
				j = nsim % 26;
				for (i = 0; i<20; i++)
					msg2give.data[i] = 97 + j;
				msg2give.data[19] = 0;
				if (TRACE>2) {
					printf("\t\t\tMAINLOOP: data given to student: ");
					for (i = 0; i<20; i++)
						printf("%c", msg2give.data[i]);
					printf("\n");
				}
				nsim++;
				if (eventptr->eventity==A)
					A_output(msg2give);
				else
					B_output(msg2give);
			}
		}
		else if (eventptr->evtype==FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i<20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity==A)    // Deliver packet by calling appropriate entity
				A_input(pkt2give);
			else
				B_input(pkt2give);
			free(eventptr->pktptr); // Free the memory for packet
		}
		else if (eventptr->evtype==TIMER_INTERRUPT) {
			if (eventptr->eventity==A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	terminate:
	printf(
		"\nSimulator terminated at time %f\nafter sending %d msgs from layer5.\n",
		time, nsim);
}

/**
 * Initialize the simulator.
 */
void init () {
	int i;
	float sum, avg;
	float jimsrand ();

	if (kDebugMode==1) {
		ReadInputFile();
	}
	else {
		printf("-------- Stop and Wait Network Simulator Version 1.1 --------\n\n");
		printf("Enter the number of messages to simulate: ");
		scanf("%d", &nsimmax);
		printf("Enter packet loss probability [enter 0.0 for no loss]:");
		scanf("%f", &lossprob);
		printf("Enter packet corruption probability [0.0 for no corruption]:");
		scanf("%f", &corruptprob);
		printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
		scanf("%f", &lambda);
		printf("Enter TRACE:");
		scanf("%d", &TRACE);
	}

	srand(9999); // Initialize random number generator
	sum = 0.0; // Test random number generator for students */
	for (i = 0; i<1000; i++)
		sum = sum + jimsrand(); // jimsrand() should be uniform in [0,1]
	avg = sum / 1000.0;
	if (avg<0.25 || avg>0.75) {
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects. Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(1);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = 0.0;              // Initialize time to 0.0
	generate_next_arrival(); // Initialize event list
}

/**
 * jimsrand(): return a float in range [0,1]. The routine below is used to isolate all random number generation in one location. We assume that the system-supplied rand() function return an int in the range [0,mmm]
 * @return
 */
float jimsrand (void) {
	double mmm = RAND_MAX;
	float x;            // Individual students may need to change mmm
	x = rand() / mmm;   // x should be uniform in [0,1]
	return (x);
}

// ******************************* EVENT HANDLING ROUTINES ******************************
// ******************* The next set of routines handle the event list *******************
// **************************************************************************************

void generate_next_arrival (void) {
	double x, log (), ceil ();
	struct event *evptr;
	float ttime;
	int tempint;

	if (TRACE>2)
		printf("\t\t\tGENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda * jimsrand() * 2; // x is uniform on [0,2*lambda] having mean of lambda
	evptr = (struct event *) malloc(sizeof(struct event));
	evptr->evtime = time + x;
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand()>0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}

void insertevent (struct event *p) {
	struct event *q, *qold;

	if (TRACE>2) {
		printf("\t\t\t\tINSERTEVENT: time is %lf\n", time);
		printf("\t\t\t\tINSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;      // q points to header of list in which p struct inserted
	if (q==NULL) {
		// List is empty
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q!=NULL && p->evtime>q->evtime; q = q->next)
			qold = q;
		if (q==NULL) {
			// End of list
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q==evlist) {
			// Front of list
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {
			// Middle of list
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

void printevlist (void) {
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q!=NULL; q = q->next) {
		printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
		       q->eventity);
	}
	printf("--------------\n");
}

// ****************************** STUDENT-CALLABLE ROUTINES *****************************

/**
 * Called by students routine to cancel a previously-started timer.
 * @param AorB A or B is trying to stop timer.
 */
void stoptimer (int AorB) {
	struct event *q, *qold;

	if (TRACE>2)
		printf("\t\t\tSTOP TIMER: stopping timer at %f\n", time);
	// for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)
	for (q = evlist; q!=NULL; q = q->next)
		if ((q->evtype==TIMER_INTERRUPT && q->eventity==AorB)) {
			// Remove this event
			if (q->next==NULL && q->prev==NULL) {
				// Remove first and only event on list
				evlist = NULL;
			}
			else if (q->next==NULL) {
				// End of list - there is one in front
				q->prev->next = NULL;
			}
			else if (q==evlist) {
				// Front of list - there must be event after
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {
				// Middle of list
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

/**
 * @param AorB A or B is trying to start timer.
 * @param increment
 */
void starttimer (int AorB, float increment) {
	struct event *q;
	struct event *evptr;

	if (TRACE>2)
		printf("\t\t\tSTART TIMER: starting timer at %f\n", time);
	// Be nice: check to see if timer is already started, if so, then warn
	// for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)
	for (q = evlist; q!=NULL; q = q->next)
		if ((q->evtype==TIMER_INTERRUPT && q->eventity==AorB)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	// Create future event for when timer goes off
	evptr = (struct event *) malloc(sizeof(struct event));
	evptr->evtime = time + increment;
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}

// ************************************** TOLAYER3 **************************************
/**
 * Sends a packet to layer 3.
 * @param AorB If 0, then the calling entity is A. If 1, then B is the calling entity.
 * @param packet The packet to be sent.
 */
void tolayer3 (int AorB, struct pkt packet) {
	struct pkt *mypktptr;
	struct event *evptr, *q;
	float lastime, x;
	int i;

	ntolayer3++;

	// Simulate losses
	if (jimsrand()<lossprob) {
		nlost++;
		if (TRACE>0)
			printf("\t\t\tTOLAYER3: packet being lost\n");
		return;
	}

	// Make a copy of the packet student just gave me since he/she may decide to do something with the packet after we return back to him/her
	mypktptr = (struct pkt *) malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i = 0; i<20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE>2) {
		printf("\t\t\tTOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
		       mypktptr->acknum, mypktptr->checksum);
		for (i = 0; i<20; i++)
			printf("%c", mypktptr->payload[i]);
		printf("\n");
	}

	// Create future event for arrival of packet at the other side
	evptr = (struct event *) malloc(sizeof(struct event));
	evptr->evtype = FROM_LAYER3;      // Packet will pop out from layer3
	evptr->eventity = (AorB + 1) % 2; // Event occurs at other entity
	evptr->pktptr = mypktptr;         // Save ptr to my copy of packet

	// Finally, compute the arrival time of packet at the other end. medium can not reorder, so make sure packet arrives between 1 and 10 time units after the latest arrival time of packets currently in the medium on their way to the destination
	lastime = time;
	// for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)
	for (q = evlist; q!=NULL; q = q->next)
		if ((q->evtype==FROM_LAYER3 && q->eventity==evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 1 + 9 * jimsrand();

	// Simulate corruption
	if (jimsrand()<corruptprob) {
		ncorrupt++;
		if ((x = jimsrand())<.75)
			mypktptr->payload[0] = 'Z'; // Corrupt payload
		else if (x<.875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE>0)
			printf("\t\t\tTOLAYER3: packet being corrupted\n");
	}

	if (TRACE>2)
		printf("\t\t\tTOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

// ************************************** TOLAYER5 **************************************
/**
 * Sends data to layer 5.
 * @param AorB Currently unused.
 * @param datasent The data to be sent.
 */
void tolayer5 (int AorB, char datasent[20]) {
	int i;
	if (TRACE>2) {
		printf("\t\t\tTOLAYER5: data received: ");
		for (i = 0; i<20; i++)
			printf("%c", datasent[i]);
		printf("\n");
	}
}

/**
 * Calculates and returns the checksum of a packet.
 * @param packet The packet of which the checksum is being calculated.
 * @return Checksum for the packet.
 */
int CalculateCheckSum (struct pkt packet) {
	int checksum = packet.seqnum + packet.acknum;
	for (int i = 0; i<kDataSize; i++) checksum += packet.payload[i];
	return checksum;
}

/**
 * Reads input file in debug mode.
 */
void ReadInputFile () {
	FILE *fp;
	char line[1000];
	fp = fopen(kDebugInputFile, "r");

	if (fp==NULL) {
		printf("Error while reading input file.\n");
		exit(EXIT_FAILURE);
	}
	else {
		fgets(line, sizeof(line), fp); // Ignore the first line.

		fgets(line, sizeof(line), fp); // Read nsimmax.
		nsimmax = atoi(line);

		fgets(line, sizeof(line), fp); // Read lossprob.
		lossprob = atof(line);

		fgets(line, sizeof(line), fp); // Read corruptprob.
		corruptprob = atof(line);

		fgets(line, sizeof(line), fp); // Read lambda.
		lambda = atof(line);

		fgets(line, sizeof(line), fp); // Read TRACE.
		TRACE = atoi(line);

		printf("nsimmax: %d\nlossprob: %f\ncorruptprob: %f\nlambda: %f\nTRACE: %d\n",
		       nsimmax,
		       lossprob,
		       corruptprob,
		       lambda,
		       TRACE);
	}

	fclose(fp);
}
