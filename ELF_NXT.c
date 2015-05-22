#pragma config(Sensor, S1,     Arduino,        sensorI2CCustom)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

//
// EPS NXT/Arduino LaserTag
// MK, GM
//



#define ARDUINO_ADDRESS 0x14    // 0x0A << 1
#define ARDUINO_PORT 		S1


//
// These must be the same as in the Arduino code
//
#define COMMAND_REPORT_HITS         1
#define COMMAND_FIRE_LASER          2
#define COMMAND_REPORT_CALIBRATION  5
#define COMMAND_REPORT_VALUE        6

#define ELF_MSG_HEADER    				  199  // unlikely special value to msg header id


unsigned int g_uTotalHits;

char auWriteBuffer[5];
char acReadBuffer[4];

int ArduinoCommand (int command)
{
	memset(acReadBuffer, 0, sizeof(acReadBuffer));

	auWriteBuffer[0] = 5;    						// Messsage Size
	auWriteBuffer[1] = ARDUINO_ADDRESS;
	auWriteBuffer[2] = ELF_MSG_HEADER;	// our header
	auWriteBuffer[3] = command;				 	// Command
	auWriteBuffer[4] = (0-command)%256;	// checksum

	sendI2CMsg(ARDUINO_PORT, auWriteBuffer, 4);
	delay(20);
	readI2CReply(ARDUINO_PORT, acReadBuffer, 4);
	writeDebugStreamLine("Result %02X %02X %02X %02X...", acReadBuffer[0], acReadBuffer[1], acReadBuffer[2], acReadBuffer[3]);

	//
	// test header
	//
	if (acReadBuffer[0] != ELF_MSG_HEADER)
	{
		//
		// not our special value -> message invalid
		//
		writeDebugStreamLine ("ELF response header error %u", acReadBuffer[0]);
		return (0);
	}

	//
	// test checksum
	//

	if  ((((byte)(acReadBuffer[1] + acReadBuffer[2] + acReadBuffer[3]))%256) != 0)
	{
		//
		// checksum invalid -> message invalid
		//
		writeDebugStreamLine ("ELF response checksum error %u", (byte)(acReadBuffer[1] + acReadBuffer[2] + acReadBuffer[3]));
		return (0);
	}

	return (((unsigned int) acReadBuffer[1]) + (((unsigned int) acReadBuffer[2])<<8));
}


task main()
{
	unsigned int x;
	g_uTotalHits = 0;

	nVolume = 4;
	clearSounds();
	eraseDisplay();
	displayCenteredBigTextLine(1, "Hits:");
	displayCenteredBigTextLine(4, "0");

	while(true)
	{
		clearDebugStream();

		if (nNxtButtonPressed != -1)
		{
			writeDebugStreamLine("Button %u", nNxtButtonPressed);
			if (nNxtButtonPressed == 3)
			{
				writeDebugStreamLine("Sending fire command ...");
				ArduinoCommand(COMMAND_FIRE_LASER);
				clearSounds();
				playSoundFile("ELF_Laser.rso");
				delay (250);
			}
		}


		/*
		** debugging code follows

		writeDebugStreamLine("Result %u...", x);
		writeDebugStreamLine(" ");

		writeDebugStreamLine("Requesting hit report ...");

		x = ArduinoCommand(COMMAND_REPORT_CALIBRATION);
		writeDebugStreamLine("Calibration %u ...", x);

		x = ArduinoCommand(COMMAND_REPORT_VALUE);
		writeDebugStreamLine("Current value %u ...", x);

		*/

		x = ArduinoCommand(COMMAND_REPORT_HITS);
		writeDebugStreamLine("Last recorded hits %u ...", x);


		if (x > 0 && x < 100)
		{
			// we were hit
			g_uTotalHits += x;
		  clearSounds();
			playSoundFile("ELF_Hit.rso");
			eraseDisplay();
			displayCenteredBigTextLine(1, "Hits:");
			displayCenteredBigTextLine(4, "%u", g_uTotalHits);
		}

		writeDebugStreamLine("Total recorded hits %u ...", g_uTotalHits);
		writeDebugStreamLine(" ");

		delay(50);
	}
}