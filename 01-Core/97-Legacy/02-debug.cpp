// legacy-reference; unchanged excerpt from OLD/[1] algorithms.cpp:31-228.
// Legacy contracts and comments await audit; see this folder's 00-index.md.

namespace Debug {
    using std::to_string;

    string to_string(bool x) { 
        return x ? "true" : "false"; 
    }

    string to_string(char x) { 
        return string({'\'', x, '\''}); 
    }

    string to_string(std::string_view x) {
        string res;
        res.reserve(x.size() + 2);
        res += '"';
        res += x;
        res += '"';
    
        return res;
    }

    string to_string(const char *x) { 
        return to_string(std::string_view(x)); 
    }

    template<size_t N> 
    string to_string(const bitset<N> &x) {
        return x.to_string(); 
    }

    template<typename T> 
    requires requires { std::tuple_size<T>::value; } 
          && (!std::ranges::range<T>)
    string to_string(const T &x);

    template<std::ranges::range T> 
    requires (!std::is_convertible_v<T, std::string_view>)
    string to_string(const T &x);

    template<typename T>
    requires (!std::ranges::range<T>) 
          && requires (std::ostream &os, const T &x) { os << x; }
    string to_string(const T &x) {
        std::stringstream ss;
        ss << x;
        
        return ss.str();
    }

    struct Any { 
        template<typename T> operator T() const; 
    };
    
    // optional
    template<typename T>
    requires std::is_aggregate_v<T> 
          && (!std::ranges::range<T>) 
          && (!requires (std::ostream &os, const T &x) { os << x; })
    string to_string(const T &x) {
        if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f, g, h] = x; 
            return to_string(std::tie(a, b, c, d, e, f, g, h));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f, g] = x; 
            return to_string(std::tie(a, b, c, d, e, f, g));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e, f] = x; 
            return to_string(std::tie(a, b, c, d, e, f));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d, e] = x; 
            return to_string(std::tie(a, b, c, d, e));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c, d] = x; 
            return to_string(std::tie(a, b, c, d));
        } else if constexpr (requires { T{Any{}, Any{}, Any{}}; }) {
            auto &[a, b, c] = x; 
            return to_string(std::tie(a, b, c));
        } else if constexpr (requires { T{Any{}, Any{}}; }) {
            auto &[a, b] = x; 
            return to_string(std::tie(a, b));
        } else if constexpr (requires { T{Any{}}; }) {
            auto &[a] = x;
            return to_string(std::tie(a));
        } else {
            return "{}";
        }
    }

    template<typename T>
    requires requires { std::tuple_size<T>::value; } 
          && (!std::ranges::range<T>)
    string to_string(const T &x) {
        string res = "(";
        std::apply(
            [&](const auto &...args) {
                int i = 0;
                ((res += (i++ ? ", " : ""), res += to_string(args)), ...);
            }, x
        );

        return res + ")";
    }

    template<typename T, typename C>
    string to_string(const queue<T, C> &x) {
        struct Accessor : queue<T, C> { 
            using queue<T, C>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<typename T, typename C>
    string to_string(const stack<T, C> &x) {
        struct Accessor : stack<T, C> { 
            using stack<T, C>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<typename T, typename C, typename Comp>
    string to_string(const priority_queue<T, C, Comp> &x) {
        struct Accessor : priority_queue<T, C, Comp> { 
            using priority_queue<T, C, Comp>::c; 
        };
        
        return to_string(static_cast<const Accessor&>(x).c);
    }

    template<std::ranges::range T> 
    requires (!std::is_convertible_v<T, std::string_view>)
    string to_string(const T &x) {
        string res = "{";
        auto it = std::begin(x);
        if (it != std::end(x)) {
            res += to_string(*it);
            for (++it; it != std::end(x); ++it) {
                res += ", ";
                res += to_string(*it);
            }
        }
        res += "}";
        
        return res;
    }

    template<typename ...Args>
    void debugO(const Args &...args) {
        ((cerr << ' ' << to_string(args)), ...);
        cerr << '\n';
    }

    template<std::ranges::range R, typename ...Args>
    auto slice(R &&ran, int l, int r, Args ...args) {
        auto v = std::forward<R>(ran) | std::views::drop(l) | std::views::take(r - l + 1);
        if constexpr (sizeof...(args) == 0) {
            return v;
        } else {
            return v | std::views::transform([=](auto &&cur) { return slice(cur, args...); });
        }
    }

    int dep = 0;

    std::string_view indent() {
        static constexpr auto spaces = [] {
            array<char, 128> v{};
            v.fill(' ');
            return v;
        }();
    
        return std::string_view(spaces.data(), min<int>(2 * dep, spaces.size()));
    }
    
    struct Tracer {
        string v; 

        Tracer(string x) : v(std::move(x)) { 
            cerr << indent() << ">> " << v << '\n'; 
            dep++; 
        }

        ~Tracer() { 
            dep--; 
            cerr << indent() << "<< " << v << '\n'; 
        }
    };
}

#ifdef LOCAL
#define debug(...) cerr << Debug::indent() << "[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:", Debug::debugO(__VA_ARGS__)
// #define debug(...) cerr << Debug::indent() << "\033[1;31m[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:\033[0m", Debug::debugO(__VA_ARGS__)
#define trace(x) Debug::Tracer _trace_guard(x)
#else
#define debug(...) void(0)
#define trace(x) void(0)
#endif
