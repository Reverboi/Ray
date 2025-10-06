#pragma once
#include <iostream>
#include <stdexcept>
#include <vector>

template <typename T>
class Buffer2D {
   public:
    using size_type = std::size_t;

    Buffer2D(size_type rows, size_type cols, const T& default_value = T{})
        : m_rows(rows), m_cols(cols), m_data(rows * cols, default_value) {}

    // Access without bounds checking
    T& operator()(size_type row, size_type col) { return m_data[row * m_cols + col]; }

    const T& operator()(size_type row, size_type col) const { return m_data[row * m_cols + col]; }

    // Safe access with bounds checking
    T& at(size_type row, size_type col) {
        check_bounds(row, col);
        return (*this)(row, col);
    }

    const T& at(size_type row, size_type col) const {
        check_bounds(row, col);
        return (*this)(row, col);
    }

    size_type rows() const noexcept { return m_rows; }
    size_type cols() const noexcept { return m_cols; }

    void fill(const T& value) { std::fill(m_data.begin(), m_data.end(), value); }

    // Optional: raw access
    T* data() noexcept { return m_data.data(); }
    const T* data() const noexcept { return m_data.data(); }

    Buffer2D(const Buffer2D&) = default;
    Buffer2D(Buffer2D&&) noexcept = default;
    Buffer2D& operator=(const Buffer2D&) = default;
    Buffer2D& operator=(Buffer2D&&) noexcept = default;

   private:
    size_type m_rows, m_cols;
    std::vector<T> m_data;

    void check_bounds(size_type row, size_type col) const {
        if (row >= m_rows || col >= m_cols) {
            throw std::out_of_range("Buffer2D index out of bounds");
        }
    }
};
