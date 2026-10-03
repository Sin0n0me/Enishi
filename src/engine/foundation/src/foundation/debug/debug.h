

#if !defined(NDEBUG) || defined(_DEBUG) || defined(DEBUG)
namespace enishi::foundation {
    constexpr bool IS_DEBUG_MODE = true;
}
#else
namespace enishi::foundation {
    constexpr bool IS_DEBUG_MODE = false;
}
#endif


namespace enishi::foundation {

}