#include <game/editor/editor.h>

CLayerTele::CLayerTele(int w, int h) :
	CLayerTiles(w, h)
{
	str_copy(m_aName, "Tele");
	m_Tele = 1;

	m_pTeleTile = new CTeleTile[w * h];
	mem_zero(m_pTeleTile, (size_t)w * h * sizeof(CTeleTile));
}

CLayerTele::~CLayerTele()
{
	delete[] m_pTeleTile;
}

void CLayerTele::Resize(int NewW, int NewH)
{
	// resize tele data
	CTeleTile *pNewTeleData = new CTeleTile[NewW * NewH];
	mem_zero(pNewTeleData, (size_t)NewW * NewH * sizeof(CTeleTile));

	// copy old data
	for(int y = 0; y < minimum(NewH, m_Height); y++)
		mem_copy(&pNewTeleData[y * NewW], &m_pTeleTile[y * m_Width], minimum(m_Width, NewW) * sizeof(CTeleTile));

	// replace old
	delete[] m_pTeleTile;
	m_pTeleTile = pNewTeleData;

	// resize tile data
	CLayerTiles::Resize(NewW, NewH);

	// resize gamelayer too
	if(m_pEditor->m_Map.m_pGameLayer->m_Width != NewW || m_pEditor->m_Map.m_pGameLayer->m_Height != NewH)
		m_pEditor->m_Map.m_pGameLayer->Resize(NewW, NewH);
}

void CLayerTele::Shift(int Direction)
{
	CLayerTiles::Shift(Direction);
	ShiftImpl(m_pTeleTile, Direction, m_pEditor->m_ShiftBy);
}

bool CLayerTele::IsEmpty(const std::shared_ptr<CLayerTiles> &pLayer)
{
	for(int y = 0; y < pLayer->m_Height; y++)
		for(int x = 0; x < pLayer->m_Width; x++)
			if(m_pEditor->m_AllowPlaceUnusedTiles || IsValidTeleTile(pLayer->GetTile(x, y).m_Index))
				return false;

	return true;
}

void CLayerTele::BrushDraw(std::shared_ptr<CLayer> pBrush, float wx, float wy)
{
	if(m_Readonly)
		return;

	std::shared_ptr<CLayerTele> pTeleLayer = std::static_pointer_cast<CLayerTele>(pBrush);
	int sx = ConvertX(wx);
	int sy = ConvertY(wy);
	if(str_comp(pTeleLayer->m_aFileName, m_pEditor->m_aFileName))
		m_pEditor->m_TeleNumber = pTeleLayer->m_TeleNum;

	bool Destructive = m_pEditor->m_BrushDrawDestructive || IsEmpty(pTeleLayer);

	for(int y = 0; y < pTeleLayer->m_Height; y++)
		for(int x = 0; x < pTeleLayer->m_Width; x++)
		{
			int fx = x + sx;
			int fy = y + sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(!Destructive && GetTile(fx, fy).m_Index)
				continue;

			if((m_pEditor->m_AllowPlaceUnusedTiles || IsValidTeleTile(pTeleLayer->m_pTiles[y * pTeleLayer->m_Width + x].m_Index)) && pTeleLayer->m_pTiles[y * pTeleLayer->m_Width + x].m_Index != TILE_AIR)
			{
				if(!IsTeleTileNumberUsed(pTeleLayer->m_pTiles[y * pTeleLayer->m_Width + x].m_Index))
				{
					// Tele tile number is unused. Set a known value which is not 0,
					// as tiles with number 0 would be ignored by previous versions.
					m_pTeleTile[fy * m_Width + fx].m_Number = 255;
				}
				else if(m_pEditor->m_TeleNumber != pTeleLayer->m_TeleNum)
				{
					m_pTeleTile[fy * m_Width + fx].m_Number = m_pEditor->m_TeleNumber;
				}
				else if(pTeleLayer->m_pTeleTile[y * pTeleLayer->m_Width + x].m_Number)
				{
					m_pTeleTile[fy * m_Width + fx].m_Number = pTeleLayer->m_pTeleTile[y * pTeleLayer->m_Width + x].m_Number;
				}
				else
				{
					if(!m_pEditor->m_TeleNumber)
					{
						m_pTeleTile[fy * m_Width + fx].m_Number = 0;
						m_pTeleTile[fy * m_Width + fx].m_Type = 0;
						m_pTiles[fy * m_Width + fx].m_Index = 0;
						continue;
					}
					else
						m_pTeleTile[fy * m_Width + fx].m_Number = m_pEditor->m_TeleNumber;
				}

				m_pTeleTile[fy * m_Width + fx].m_Type = pTeleLayer->m_pTiles[y * pTeleLayer->m_Width + x].m_Index;
				m_pTiles[fy * m_Width + fx].m_Index = pTeleLayer->m_pTiles[y * pTeleLayer->m_Width + x].m_Index;
			}
			else
			{
				m_pTeleTile[fy * m_Width + fx].m_Number = 0;
				m_pTeleTile[fy * m_Width + fx].m_Type = 0;
				m_pTiles[fy * m_Width + fx].m_Index = 0;
			}
		}
	FlagModified(sx, sy, pTeleLayer->m_Width, pTeleLayer->m_Height);
}

void CLayerTele::BrushFlipX()
{
	CLayerTiles::BrushFlipX();
	BrushFlipXImpl(m_pTeleTile);
}

void CLayerTele::BrushFlipY()
{
	CLayerTiles::BrushFlipY();
	BrushFlipYImpl(m_pTeleTile);
}

void CLayerTele::BrushRotate(float Amount)
{
	int Rotation = (round_to_int(360.0f * Amount / (pi * 2)) / 90) % 4; // 0=0°, 1=90°, 2=180°, 3=270°
	if(Rotation < 0)
		Rotation += 4;

	if(Rotation == 1 || Rotation == 3)
	{
		// 90° rotation
		CTeleTile *pTempData1 = new CTeleTile[m_Width * m_Height];
		CTile *pTempData2 = new CTile[m_Width * m_Height];
		mem_copy(pTempData1, m_pTeleTile, (size_t)m_Width * m_Height * sizeof(CTeleTile));
		mem_copy(pTempData2, m_pTiles, (size_t)m_Width * m_Height * sizeof(CTile));
		CTeleTile *pDst1 = m_pTeleTile;
		CTile *pDst2 = m_pTiles;
		for(int x = 0; x < m_Width; ++x)
			for(int y = m_Height - 1; y >= 0; --y, ++pDst1, ++pDst2)
			{
				*pDst1 = pTempData1[y * m_Width + x];
				*pDst2 = pTempData2[y * m_Width + x];
			}

		std::swap(m_Width, m_Height);
		delete[] pTempData1;
		delete[] pTempData2;
	}

	if(Rotation == 2 || Rotation == 3)
	{
		BrushFlipX();
		BrushFlipY();
	}
}

void CLayerTele::FillSelection(bool Empty, std::shared_ptr<CLayer> pBrush, CUIRect Rect)
{
	if(m_Readonly || (!Empty && pBrush->m_Type != LAYERTYPE_TILES))
		return;

	Snap(&Rect); // corrects Rect; no need of <=

	Snap(&Rect);

	int sx = ConvertX(Rect.x);
	int sy = ConvertY(Rect.y);
	int w = ConvertX(Rect.w);
	int h = ConvertY(Rect.h);

	std::shared_ptr<CLayerTele> pLt = std::static_pointer_cast<CLayerTele>(pBrush);

	bool Destructive = m_pEditor->m_BrushDrawDestructive || Empty || IsEmpty(pLt);

	for(int y = 0; y < h; y++)
	{
		for(int x = 0; x < w; x++)
		{
			int fx = x + sx;
			int fy = y + sy;

			if(fx < 0 || fx >= m_Width || fy < 0 || fy >= m_Height)
				continue;

			if(!Destructive && GetTile(fx, fy).m_Index)
				continue;

			const int SrcIndex = Empty ? 0 : (y * pLt->m_Width + x % pLt->m_Width) % (pLt->m_Width * pLt->m_Height);
			const int TgtIndex = fy * m_Width + fx;

			if(Empty || (!m_pEditor->m_AllowPlaceUnusedTiles && !IsValidTeleTile((pLt->m_pTiles[SrcIndex]).m_Index)))
			{
				m_pTiles[TgtIndex].m_Index = 0;
				m_pTeleTile[TgtIndex].m_Type = 0;
				m_pTeleTile[TgtIndex].m_Number = 0;
			}
			else
			{
				m_pTiles[TgtIndex] = pLt->m_pTiles[SrcIndex];
				if(pLt->m_Tele && m_pTiles[TgtIndex].m_Index > 0)
				{
					m_pTeleTile[TgtIndex].m_Type = m_pTiles[TgtIndex].m_Index;

					if(!IsTeleTileNumberUsed(m_pTeleTile[TgtIndex].m_Type))
					{
						// Tele tile number is unused. Set a known value which is not 0,
						// as tiles with number 0 would be ignored by previous versions.
						m_pTeleTile[TgtIndex].m_Number = 255;
					}
					else if((pLt->m_pTeleTile[SrcIndex].m_Number == 0 && m_pEditor->m_TeleNumber) || m_pEditor->m_TeleNumber != pLt->m_TeleNum)
						m_pTeleTile[TgtIndex].m_Number = m_pEditor->m_TeleNumber;
					else
						m_pTeleTile[TgtIndex].m_Number = pLt->m_pTeleTile[SrcIndex].m_Number;
				}
			}
		}
	}
	FlagModified(sx, sy, w, h);
}

bool CLayerTele::ContainsElementWithId(int Id)
{
	for(int y = 0; y < m_Height; ++y)
	{
		for(int x = 0; x < m_Width; ++x)
		{
			if(IsTeleTileNumberUsed(m_pTeleTile[y * m_Width + x].m_Type) && m_pTeleTile[y * m_Width + x].m_Number == Id)
			{
				return true;
			}
		}
	}

	return false;
}
