#ifndef alg_analysis_h
#define alg_analysis_h

enum class NSGAIIIAnalysis
{
	None                   = 0b0000,
	Entropy                = 0b0001,
	ElitesUsedAndUpdated   = 0b0010,
	ElitesUpdateTracking   = 0b0100,
	ObjValIterationSetter  = 0b1000
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
