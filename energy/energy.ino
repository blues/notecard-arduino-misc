
// Include the Arduino library for the Notecard
#include <Notecard.h>
#include <Wire.h>

// Note that both of these definitions are optional; just prefix either line with // to remove it.
//  Remove serialNotecard if you wired your Notecard using I2C SDA/SCL pins instead of serial RX/TX
//  Remove serialTrace if you don't want the Notecard library to output debug information
#define serialTrace Serial
// #define serialNotecard Serial1

#ifndef PRODUCT_UID
#define PRODUCT_UID ""
#endif

// This is the unique Product Identifier for your device.
#define myProductID PRODUCT_UID
Notecard notecard;

// Control parameters
#define	NUM_SECONDS_BETWEEN_CYCLES		15
#define	NUM_RETRIES_FOR_A_GIVEN_PAYLOAD 3

// Vars that control cycle behavior
uint32_t payloadLen = 0;
uint32_t cycleDelaySecs = 0;
uint32_t cycleRetriesLeft = 0;
uint32_t lastSyncDurationSecs = 0;

// One-time Arduino initialization
void setup()
{

    // During development, set up for debug output from the Notecard.  Note that the initial delay is
    // required by some Arduino cards before debug UART output can be successfully displayed in the
    // Arduino IDE, including the Adafruit Feather nRF52840 Express.
#ifdef serialTrace
    delay(2500);
    serialTrace.begin(115200);
    notecard.setDebugOutputStream(serialTrace);
#endif

    // Initialize the physical I/O channel to the Notecard
#ifdef serialNotecard
    notecard.begin(serialNotecard, 9600);
#else
    Wire.begin();
    notecard.begin();
#endif

    // Set the product UID if defined and set the sync mode to be "minimum"
    J *req = notecard.newRequest("hub.set");
	if (myProductID[0]) {
		JAddStringToObject(req, "product", myProductID);
	}
	JAddStringToObject(req, "mode", "minimum");
	notecard.sendRequest(req);
	// Perform at least three sync cycles just to ensure that we've gotten past our
	// initial connectivity and secure authentication, which takes more bandwidth and time
	for (int i=5; i>0 && (lastSyncDurationSecs == 0 || lastSyncDurationSecs > 16); --i) {
		serialTrace.printf("\n\n*** syncing so that notecard is in a stable state (%d)\n", i);
		payloadLen = 0;
		cycleDelaySecs = 0;
		loop();
	}

	// Begin, and cycle with a delay
	payloadLen = 0;
	cycleDelaySecs = NUM_SECONDS_BETWEEN_CYCLES;
	cycleRetriesLeft = NUM_RETRIES_FOR_A_GIVEN_PAYLOAD;
	serialTrace.printf("\n\n*** beginning test (YOU HAVE 10 SECONDS TO START RECORDING)\n\n\n");
	delay(10000);

}

// The Arduino main loop does a single cycle, and then delays for viewing results on the scope
void loop()
{

	// Begin a cycle
	serialTrace.printf("*** beginning cycle with %d-byte payload\n", payloadLen);

    // Enqueue the measurement to the Notecard for transmission to the Notehub.  These measurements
    // will be staged in the Notecard's flash memory until it's time to transmit them to the service.
	if (payloadLen > 0) {
		serialTrace.printf("*** adding note\n");
	    J *req = notecard.newRequest("note.add");
	    if (req != NULL) {

			// Notefile
			JAddStringToObject(req, "file", "test_outbound.qo");

			// This field allows, for testing, payloads of arbitrary length
			JAddBoolToObject(req, "allow", true);

			// Generate an uncompressable payload of the desired length
			uint8_t *payload = (uint8_t *) malloc(payloadLen);
			if (payload != NULL) {
				for (uint32_t i=0; i<payloadLen; i++) {
					payload[i] = i == 0 ? 1 : (payload[i-1] * i) + i;
				}
				JAddBinaryToObject(req, "payload", payload, payloadLen);
				free(payload);
			}
	        notecard.sendRequest(req);
	    }
	}

	// Initiate a sync
	notecard.sendRequest(notecard.newRequest("hub.sync"));

	// Loop, waiting for the sync to be completed
	serialTrace.printf("*** waiting for sync to be completed\n");
	bool syncError = false;
	int syncDurationSecs = 0;
	bool syncInProgress = true;
	while (syncInProgress) {
		delay(3000);
	    J *rsp = notecard.requestAndResponse(notecard.newRequest("hub.sync.status"));
		if (rsp == NULL) {
			break;
		}
		syncError = JGetBool(rsp, "alert");
		syncDurationSecs = JGetNumber(rsp, "duration");
		syncInProgress = JGetNumber(rsp, "requested") > 0;
        notecard.deleteResponse(rsp);
	}
	lastSyncDurationSecs = syncDurationSecs;
	serialTrace.printf("*** sync is completed %s\n", syncError ? "(CONNECTIVITY ERROR)" : "");

	// Process any inbound messages
	for (;;) {
	    J *req = notecard.newRequest("note.get");
	    if (req != NULL) {

			// Notefile
			JAddStringToObject(req, "file", "test_inbound.qi");

			// Delete the note as a side-effect of the GET
			JAddBoolToObject(req, "delete", true);

			// Dequeue the next inbound note
		    J *rsp = notecard.requestAndResponse(req);
			if (rsp != NULL) {

				// Done?
				if (!JIsNullString(rsp, "err")) {
			        notecard.deleteResponse(rsp);
					break;
				}

				// Process the inbound note in "rsp"
				serialTrace.printf("*** processing inbound message\n");

				// Done with processing
		        notecard.deleteResponse(rsp);

			}
		}
	}

	// Bump the payload to the next payload size
	if (cycleRetriesLeft > 0) {
		cycleRetriesLeft--;
	} else {
		payloadLen = payloadLen * 10;
		if (payloadLen == 0) {
			payloadLen = 1;
		}
		if (payloadLen > 10000) {
			payloadLen = 0;
		}
		cycleRetriesLeft = NUM_RETRIES_FOR_A_GIVEN_PAYLOAD;
	}

	// Delay between cycles as desired
	if (cycleDelaySecs > 0) {
		serialTrace.printf("*** done with cycle (delaying %d seconds before uploading %d-byte payload)\n\n",
						   cycleDelaySecs, payloadLen);
		delay(cycleDelaySecs * 1000);
	}

}
