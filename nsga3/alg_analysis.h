#ifndef alg_analysis_h
#define alg_analysis_h

enum class NSGAIIIAnalysis
{
	None                   = 0b0,
	Entropy                = 0b1,
	ElitesUsedAndUpdated   = 0b10
};

inline NSGAIIIAnalysis operator | (NSGAIIIAnalysis lhs, NSGAIIIAnalysis rhs)
{
	using T = std::underlying_type_t <NSGAIIIAnalysis>;
	return static_cast<NSGAIIIAnalysis>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline bool operator & (NSGAIIIAnalysis lhs, NSGAIIIAnalysis rhs)
{
	using T = std::underlying_type_t <NSGAIIIAnalysis>;
	return static_cast<T>(lhs) & static_cast<T>(rhs);
}

#endif
