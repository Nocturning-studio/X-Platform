#ifndef TRIVIAL_ENCRYPTOR_H
#define TRIVIAL_ENCRYPTOR_H

//-----------------------------------------------------------------------------
// Note:
// macros TRIVIAL_ENCRYPTOR_ENCODER and/or TRIVIAL_ENCRYPTOR_DECODER
// must be defined before including this file in the implementation units.
//-----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include <algorithm> // for std::swap
#pragma warning(pop)

class trivial_encryptor
{
  public:
	// Supported encryption profiles
	enum EEncryptionProfile
	{
		PROFILE_WW = 0, // Worldwide (Europe/USA)
		PROFILE_RU = 1, // CIS (Russia/Ukraine)
		PROFILE_COUNT = 2
	};

  private:
	typedef u8 type;
	typedef void* pvoid;
	typedef const void* pcvoid;

	enum
	{
		alphabet_size = u32(1 << (8 * sizeof(type)))
	};

  private:
	// X-Ray RNG
	class random32
	{
	  private:
		u32 m_seed;

	  public:
		IC void seed(const u32& seed)
		{
			m_seed = seed;
		}

		IC u32 random(const u32& range)
		{
			m_seed = 0x08088405 * m_seed + 1;
			return (u32(u64(m_seed) * u64(range) >> 32));
		}
	};

	struct encryption_params
	{
		u32 m_table_iterations;
		u32 m_table_seed;
		u32 m_encrypt_seed;
	};

  public:
	// Parameters for all profiles
	static encryption_params m_params[PROFILE_COUNT];
	static EEncryptionProfile m_active_profile;

#if defined(TRIVIAL_ENCRYPTOR_ENCODER) && defined(TRIVIAL_ENCRYPTOR_DECODER)
  private:
	static bool m_initialized;
#endif

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
  private:
	static type m_alphabet[PROFILE_COUNT][alphabet_size];
#endif

#ifdef TRIVIAL_ENCRYPTOR_DECODER
  private:
	static type m_alphabet_back[PROFILE_COUNT][alphabet_size];
#endif

  private:
	IC static void initialize()
	{
		type* temp_alphabet = (type*)_alloca(sizeof(type) * alphabet_size);

		for (int p = 0; p < PROFILE_COUNT; ++p)
		{
			for (u32 i = 0; i < alphabet_size; ++i)
				temp_alphabet[i] = (type)i;

			random32 rnd;
			rnd.seed(m_params[p].m_table_seed);

			for (u32 i = 0; i < m_params[p].m_table_iterations; ++i)
			{
				u32 j = rnd.random(alphabet_size);
				u32 k = rnd.random(alphabet_size);
				while (j == k)
					k = rnd.random(alphabet_size);

				std::swap(temp_alphabet[j], temp_alphabet[k]);
			}

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
			for (u32 i = 0; i < alphabet_size; ++i)
				m_alphabet[p][i] = temp_alphabet[i];
#endif
#ifdef TRIVIAL_ENCRYPTOR_DECODER
			for (u32 i = 0; i < alphabet_size; ++i)
				m_alphabet_back[p][temp_alphabet[i]] = (type)i;
#endif
		}
	}

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
  public:
	// Default encoder (uses active profile)
	IC static void encode(pcvoid source, const u32& source_size, pvoid destination)
	{
		encode_specific(source, source_size, destination, m_active_profile);
	}

	// Profile-specific encoder
	IC static void encode_specific(pcvoid source, const u32& source_size, pvoid destination, EEncryptionProfile profile)
	{
#if !defined(TRIVIAL_ENCRYPTOR_DECODER)
		static bool m_initialized = false;
#endif
		if (!m_initialized)
		{
			initialize();
			m_initialized = true;
		}

		random32 rnd;
		rnd.seed(m_params[profile].m_encrypt_seed);
		const u8* I = (const u8*)source;
		const u8* E = (const u8*)source + source_size;
		u8* J = (u8*)destination;

		for (; I != E; ++I, ++J)
			*J = m_alphabet[profile][*I] ^ type(rnd.random(256) & 0xff);
	}
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef TRIVIAL_ENCRYPTOR_DECODER
  public:
	// Default decoder (uses active profile)
	IC static void decode(pcvoid source, const u32& source_size, pvoid destination)
	{
		decode_specific(source, source_size, destination, m_active_profile);
	}

	IC static void decode_rus(pcvoid source, const u32& source_size, pvoid destination)
	{
		decode_specific(source, source_size, destination, PROFILE_RU);
	}

	IC static void decode_ww(pcvoid source, const u32& source_size, pvoid destination)
	{
		decode_specific(source, source_size, destination, PROFILE_WW);
	}

	// Internal universal implementation
	IC static void decode_specific(pcvoid source, const u32& source_size, pvoid destination, EEncryptionProfile profile)
	{
#if !defined(TRIVIAL_ENCRYPTOR_ENCODER)
		static bool m_initialized = false;
#endif
		if (!m_initialized)
		{
			initialize();
			m_initialized = true;
		}

		random32 rnd;
		rnd.seed(m_params[profile].m_encrypt_seed);
		const u8* I = (const u8*)source;
		const u8* E = (const u8*)source + source_size;
		u8* J = (u8*)destination;

		for (; I != E; ++I, ++J)
			*J = m_alphabet_back[profile][(*I) ^ type(rnd.random(256) & 0xff)];
	}

	// Runtime switching (useful for writing archives)
	IC static void select_profile(EEncryptionProfile profile)
	{
		m_active_profile = profile;
	}
#endif // TRIVIAL_ENCRYPTOR_DECODER
};

// ============================================================================
// STATIC DATA STORAGE
// ============================================================================

#if defined(TRIVIAL_ENCRYPTOR_ENCODER) && defined(TRIVIAL_ENCRYPTOR_DECODER)
bool trivial_encryptor::m_initialized = false;
#endif

// Default to Worldwide keys initially
trivial_encryptor::EEncryptionProfile trivial_encryptor::m_active_profile = trivial_encryptor::PROFILE_WW;

// Key constants: { iterations, shuffle_seed, encrypt_seed }
trivial_encryptor::encryption_params trivial_encryptor::m_params[trivial_encryptor::PROFILE_COUNT] = {
	{1024, 6011979, 24031979}, // PROFILE_WW
	{2048, 20091958, 20031955} // PROFILE_RU
};

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
trivial_encryptor::type trivial_encryptor::m_alphabet[trivial_encryptor::PROFILE_COUNT]
													 [trivial_encryptor::alphabet_size];
#endif

#ifdef TRIVIAL_ENCRYPTOR_DECODER
trivial_encryptor::type trivial_encryptor::m_alphabet_back[trivial_encryptor::PROFILE_COUNT]
														  [trivial_encryptor::alphabet_size];
#endif

// -------------------------------------------------------------------------------------------------
// Universal Encryption Auto-Detection
// -------------------------------------------------------------------------------------------------

// State cache: -1 = unknown, 0 = WW, 1 = RU
static int g_last_successful_profile = -1;

// Wrapper function to determine valid keys for FS archives
static void DecodeGameResources(const void* source, const u32& size, void* destination)
{
	// 1. Short path: Block too small for analysis or impact.
	if (size < 4)
	{
		if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
			trivial_encryptor::decode_rus(source, size, destination);
		else
			trivial_encryptor::decode_ww(source, size, destination);
		return;
	}

	u8 probe_buffer[16];
	size_t probe_len = (size < 16) ? size : 16;

	// Heuristic validator: Header usually contains file count/uncompressed size.
	// Limits raised to ~128MB to support large mod archives while filtering garbage (>3GB).
	auto is_valid_header = [](u32 value) -> bool { return (value > 0) && (value < 128000000); };

	// 2. Sticky Logic: Try previously successful profile first to avoid fluctuation on large blocks
	if (g_last_successful_profile != -1)
	{
		std::memcpy(probe_buffer, source, probe_len);
		if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
			trivial_encryptor::decode_rus(probe_buffer, u32(probe_len), probe_buffer);
		else
			trivial_encryptor::decode_ww(probe_buffer, u32(probe_len), probe_buffer);

		u32 check_val = *((u32*)probe_buffer);

		if (is_valid_header(check_val))
		{
			// Confirmed valid again - proceed
			if (g_last_successful_profile == trivial_encryptor::PROFILE_RU)
				trivial_encryptor::decode_rus(source, size, destination);
			else
				trivial_encryptor::decode_ww(source, size, destination);
			return;
		}

		// Validation failed, reset cache
		Msg("![AutoDecoder]: Cached profile failed (Val: %u). Resetting detection.", check_val);
		g_last_successful_profile = -1;
	}

	// 3. Full Auto-Detection
	Msg("[AutoDecoder]: Detecting profile for block size %u...", size);

	// Test Worldwide
	std::memcpy(probe_buffer, source, probe_len);
	trivial_encryptor::decode_ww(probe_buffer, u32(probe_len), probe_buffer);
	u32 val_ww = *((u32*)probe_buffer);
	bool ww_ok = is_valid_header(val_ww);

	// Test Russian
	std::memcpy(probe_buffer, source, probe_len);
	trivial_encryptor::decode_rus(probe_buffer, u32(probe_len), probe_buffer);
	u32 val_ru = *((u32*)probe_buffer);
	bool ru_ok = is_valid_header(val_ru);

	// Decision Matrix
	if (ww_ok && !ru_ok)
	{
		Msg("[AutoDecoder]: Detected WORLDWIDE (Val: %u).", val_ww);
		g_last_successful_profile = trivial_encryptor::PROFILE_WW;
		trivial_encryptor::decode_ww(source, size, destination);
	}
	else if (!ww_ok && ru_ok)
	{
		Msg("[AutoDecoder]: Detected RUSSIAN (Val: %u).", val_ru);
		g_last_successful_profile = trivial_encryptor::PROFILE_RU;
		trivial_encryptor::decode_rus(source, size, destination);
	}
	else if (ww_ok && ru_ok)
	{
		// Ambiguous: pick smallest reasonable number
		if (val_ww < val_ru)
		{
			Msg("[AutoDecoder]: Ambiguous. Guessing WW (%u vs %u)", val_ww, val_ru);
			g_last_successful_profile = trivial_encryptor::PROFILE_WW;
			trivial_encryptor::decode_ww(source, size, destination);
		}
		else
		{
			Msg("[AutoDecoder]: Ambiguous. Guessing RU (%u vs %u)", val_ru, val_ww);
			g_last_successful_profile = trivial_encryptor::PROFILE_RU;
			trivial_encryptor::decode_rus(source, size, destination);
		}
	}
	else
	{
		// Critical failure. Fallback to WW default.
		Msg("![AutoDecoder]: CRITICAL WARNING! Unknown format (WW: %u, RU: %u).", val_ww, val_ru);
		trivial_encryptor::decode_ww(source, size, destination);
	}
}

#endif // TRIVIAL_ENCRYPTOR_H
