// S: O(n), U: NA, Q: O(1), M: O(n)
struct WelzlMEC {
    using cdb = std::complex<double>;
    using mec = std::variant<array<cdb, 2>, array<cdb, 3>>;
    static inline std::mt19937_64 rng{std::chrono::steady_clock::now().time_since_epoch().count()};

    double indicator(const mec &c, cdb p) {
        return std::visit([&](auto &&c) -> double {
            cdb a = c[0], b = c[1], i0 = (b - p) * std::conj(a - p);
            if constexpr (std::tuple_size_v<std::decay_t<decltype(c)>> == 2) { 
                return std::real(i0);
            } else { 
                cdb i2 = (a - c[2]) * std::conj(b - c[2]), i1 = i0 * i2; 
                return std::imag(i2) < 0 ? -std::imag(i1) : std::imag(i1); 
            }
        }, c);
    }

    bool isInside(const mec &c, cdb p) { return indicator(c, p) <= 0; }

    mec getMEC(vector<cdb> ps) {
        if (ps.empty()) { return array<cdb, 2>{0, 0}; }
        if (ps.size() == 1) { return array<cdb, 2>{ps[0], ps[0]}; }
        
        std::shuffle(ps.begin(), ps.end(), rng);
        mec c = array<cdb, 2>{ps[0], ps[1]};
        for (int i = 0; i < ps.size(); i++) { 
            if (!isInside(c, ps[i])) { c = array<cdb, 2>{ps[i], ps[0]};
        for (int j = 0; j < i; j++) { 
            if (!isInside(c, ps[j])) { c = array<cdb, 2>{ps[i], ps[j]};
        for (int k = 0; k < j; k++) { 
            if (!isInside(c, ps[k])) { c = array<cdb, 3>{ps[i], ps[j], ps[k]};
        }}}}}}
        return c;
    }
};