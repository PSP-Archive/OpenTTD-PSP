/* $Id: fontcache.h 7182 2006-11-16 22:05:33Z peter1138 $ */

#ifndef FONTCACHE_H
#define FONTCACHE_H

/** Get the SpriteID mapped to the given font size and key */
SpriteID GetUnicodeGlyph(FontSize size, uint32 key);

/** Map a SpriteID to the font size and key */
void SetUnicodeGlyph(FontSize size, uint32 key, SpriteID sprite);

/** Initialize the glyph map */
void InitializeUnicodeGlyphMap(void);

#ifdef WITH_FREETYPE

typedef struct FreeTypeSettings {
	char small_font[260];
	char medium_font[260];
	char large_font[260];
	uint small_size;
	uint medium_size;
	uint large_size;
} FreeTypeSettings;

extern FreeTypeSettings _freetype;

void InitFreeType(void);
const struct Sprite *GetGlyph(FontSize size, uint32 key);
uint GetGlyphWidth(FontSize size, uint32 key);

#else

/* Stub for initializiation */
static inline void InitFreeType(void) {}

/** Get the Sprite for a glyph */
static inline const Sprite *GetGlyph(FontSize size, uint32 key)
{
	SpriteID sprite = GetUnicodeGlyph(size, key);
	if (sprite == 0) sprite = GetUnicodeGlyph(size, '?');
	return GetSprite(sprite);
}


/** Get the width of a glyph */
static inline uint GetGlyphWidth(FontSize size, uint32 key)
{
	SpriteID sprite = GetUnicodeGlyph(size, key);
	if (sprite == 0) sprite = GetUnicodeGlyph(size, '?');
	return SpriteExists(sprite) ? GetSprite(sprite)->width + (size != FS_NORMAL) : 0;
}

#endif /* WITH_FREETYPE */

#endif /* FONTCACHE_H */
