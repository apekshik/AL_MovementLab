// ALHUD.cpp

#include "ALHUD.h"
#include "Engine/Canvas.h"

void AALHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowCrosshair || !Canvas)
	{
		return;
	}

	const float CX = Canvas->SizeX * 0.5f;
	const float CY = Canvas->SizeY * 0.5f;
	const FLinearColor Color = CrosshairColor;
	const float Half = CrosshairThickness * 0.5f;

	// Four lines radiating from the center gap
	DrawRect(Color, CX - CrosshairGap - CrosshairLineLength, CY - Half, CrosshairLineLength, CrosshairThickness); // left
	DrawRect(Color, CX + CrosshairGap, CY - Half, CrosshairLineLength, CrosshairThickness);                       // right
	DrawRect(Color, CX - Half, CY - CrosshairGap - CrosshairLineLength, CrosshairThickness, CrosshairLineLength); // up
	DrawRect(Color, CX - Half, CY + CrosshairGap, CrosshairThickness, CrosshairLineLength);                       // down

	if (bCenterDot)
	{
		DrawRect(Color, CX - Half, CY - Half, CrosshairThickness, CrosshairThickness);
	}
}
