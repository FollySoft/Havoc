#include "Serial.h"

//  "Havoc" - A game by jwest.
//
//	Mercy is the mark of great men.  
//	I guess we're just good men.
//
//	...well, we're alright.
//
//  JJ

int gameState = 0;

enum gameWeapons
{
	SWORD,		// Face 0
	AXE,		// Faces 0 & 1
	CHAKRAM		// Faces 0, 2, & 4
};

int gameWeapon = 0;

int swordFaces[1] = {0};
int axeFaces[2] = {0, 1};
int chakramFaces[3] = {0, 2, 4};

int damage = 0;
bool damageTaken = false;
bool damagedFace[5] = {false, false, false, false, false};

Timer healthFlashTimer;
bool healthLEDOn = false;
int healthLEDSpeed;

Color bladeColor = makeColorHSB(180, 0, 225);
int brandishBrightness = 255;
#define PULSE_LENGTH 2000


enum Flags
{
	EMPTY,
	BLADE
};

ServicePortSerial sp;

void setup()
{		
	gameState = 0;
}

void loop()
{		
	switch (gameState)
	{
		/********* Weapon Select/Forge State *********/
		case 0:
			setValueSentOnAllFaces(EMPTY);
			// Display current weapon
			switch (gameWeapon)
			{
				case SWORD:
					weaponDisplay(swordFaces, 1);					
				break;
				case AXE:
					weaponDisplay(axeFaces, 2);
				break;
				case CHAKRAM:
					weaponDisplay(chakramFaces, 3);
				break;
			}
			// Cycle through weapons
			if (buttonSingleClicked())
			{
				gameWeapon++;
				if (gameWeapon > 2) { gameWeapon = 0; }
			}
			// Start game
			if (buttonDoubleClicked())
			{
				if (isAlone())
				{
					gameState = 1;
				}
			}
		break;
		
		/********* Transition from Forge to Play State *********/
		case 1:
			setColor(OFF);
			switch (gameWeapon)
			{
				case SWORD:
					brandishWeapon(swordFaces, 1);			
				break;
				
				case AXE:
					brandishWeapon(axeFaces, 2);			
				break;

				case CHAKRAM:
					brandishWeapon(chakramFaces, 3);			
				break;
			}
			if (brandishBrightness == 225)
			{
				gameState = 2;
			}
		break;
		
		/********* Play State *********/
		case 2:
			// Listen for blade signal
			switch (gameWeapon)
			{
				case SWORD:
					weaponDetect(swordFaces, 1);
					if (damage > 0)
					{
						healthFlash(damage, swordFaces, 1);
					}
				break;
				
				case AXE:
					weaponDetect(axeFaces, 2);
					if (damage > 0)
					{
						healthFlash(damage, axeFaces, 2);
					}
				break;

				case CHAKRAM:
					weaponDetect(chakramFaces, 3);
					if (damage > 0)
					{
						healthFlash(damage, chakramFaces, 3);
					}
				break;
			}
			
			//Jump to Dead State
			if (damage == 3)
			{
				setColor(RED);
				buttonDoubleClicked();
				gameState = 3;
			}
		break;
		/********* Dead State *********/
		case 3:
			// Reset Game
			if (buttonDoubleClicked())
			{
				damage = 0;
				damageTaken = false;
				brandishBrightness = 255;
				FOREACH_FACE(f)
				{
					setColorOnFace(OFF,f);
				}
				gameState = 0;
			}
		break;
	}
}

void weaponDisplay(int weaponFaces[], int size)
{
	byte currentBrightness = max(200, random(225));
	//byte currentBrightness = 255;
	byte currentHue = map(max(40, random(50)), 0, 360, 0, 255);
	
	FOREACH_FACE(f)
	{
		for (int i = 0; i < size; i++)
		{
			// If face matches face in weapon array, set blade
			if ((f - 1) == weaponFaces[i])
			{
				/*
				// Sine wave light pulse
				int pulseProgress = millis() % PULSE_LENGTH;
				byte pulseMapped = map(pulseProgress, 0, PULSE_LENGTH, 0, 225);
				//byte dimness = sin8_C(pulseMapped);
				byte original_dimness = sin8_C(pulseMapped);

				byte new_dimness = (byte)(((int)original_dimness * (int)105 / (int)255) + (int)150);
				setColorOnFace(dim(bladeColor, new_dimness), f);
				*/
				setColorOnFace(bladeColor, f);
				goto cont;
			}
			else
			{
				setColorOnFace(makeColorHSB(currentHue, 255, currentBrightness), f);
				//setColorOnFace(OFF, f);
			}
		}
		cont:; // Continue to next face in loop
	}
}

void sparksDisplay(int face)
{
	// Set face and adjacent faces(?) to orange and back again.	
}

void brandishWeapon(int weaponFaces[], int size)
{
	FOREACH_FACE(f)
	{
		if (compareFaces(f, weaponFaces, size))
		{
			if (millis() % 3 == 0)
			{
				brandishBrightness -= 5;
				setColorOnFace(dim(bladeColor, brandishBrightness), f);
			}
		}
	}
}

void weaponDetect(int weaponFaces[], int size)
{
	FOREACH_FACE(f)
	{
		if (!compareFaces(f, weaponFaces, size))
		{
			setValueSentOnFace(EMPTY, f);
			// Take damage ONCE when blade face detected.
			if (getLastValueReceivedOnFace(f) == BLADE && 
				!isValueReceivedOnFaceExpired(f) &&
				damagedFace[f - 1] == false)
			{
				damagedFace[f - 1] = true;
				setColorOnFace(RED, f);
				damage++;
			}
			//  Allow face to be damaged again after blade expires.
			if (damagedFace[f - 1] == true &&
				isValueReceivedOnFaceExpired(f))
			{
				damagedFace[f - 1] = false;
			}
		}
		else
		{
			setValueSentOnFace(BLADE, f);
			continue;
		}
	}
}


bool compareFaces(int blinkFace, int weaponFaces[], int size)
{
	bool matchFound = false;
	for (int i = 0; i < size; i++)
	{
		if ((blinkFace - 1) == weaponFaces[i])
		{
			matchFound = true;
			return matchFound;
		}
		else { continue; }
	}
	if (matchFound == false) { return matchFound; }
}

void healthFlash(int damage, int weaponFaces[], int size)
{
	if (damage == 1) { healthLEDSpeed = 800; }		// Flash every 800 ms
	else if (damage == 2) { healthLEDSpeed = 400;}	// Flash every 400 ms
	
	if (healthFlashTimer.isExpired()) 
	{
		healthLEDOn = !healthLEDOn;
		if (healthLEDOn) 
		{ 
			FOREACH_FACE(f)
			{
				if (!compareFaces(f, weaponFaces, size)) 
				{ 
					setColorOnFace(RED, f); 
				}
			}
		}
		else
		{
			FOREACH_FACE(f)
			{
				if (!compareFaces(f, weaponFaces, size))  
				{ 
					setColorOnFace(OFF, f); 
				}
			}
		}
		healthFlashTimer.set(healthLEDSpeed); 
	}	

}
