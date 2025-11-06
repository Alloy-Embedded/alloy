/// Test file to verify register generation produces zero-overhead code
///
/// Compile with:
///   arm-none-eabi-g++ -mcpu=cortex-m3 -O2 -S test_register_generation.cpp
///
/// Expected: Assembly should be identical to manual bit manipulation

#include "../../src/hal/vendors/st/stm32f1/stm32f103xx/registers/rcc_registers.hpp"
#include "../../src/hal/vendors/st/stm32f1/stm32f103xx/bitfields/rcc_bitfields.hpp"

using namespace alloy::hal::st::stm32f1::stm32f103xx::rcc;

// Test 1: Single bit set using template
void test_template_set_bit() {
    // Enable HSE (High Speed External oscillator)
    RCC->CR = cr::HSEON::set(RCC->CR);
}

// Test 2: Manual bit set (reference)
void test_manual_set_bit() {
    // Enable HSE manually
    volatile uint32_t* rcc_cr = reinterpret_cast<volatile uint32_t*>(0x40021000);
    *rcc_cr |= (1 << 16);
}

// Test 3: Read bit field value
bool test_template_read_bit() {
    // Check if HSE is ready
    return cr::HSERDY::test(RCC->CR);
}

// Test 4: Manual read (reference)
bool test_manual_read_bit() {
    volatile uint32_t* rcc_cr = reinterpret_cast<volatile uint32_t*>(0x40021000);
    return (*rcc_cr & (1 << 17)) != 0;
}

// Test 5: Write multi-bit field
void test_template_write_field() {
    // Set HSI trim value to 16
    RCC->CR = cr::HSITRIM::write(RCC->CR, 16);
}

// Test 6: Manual write (reference)
void test_manual_write_field() {
    volatile uint32_t* rcc_cr = reinterpret_cast<volatile uint32_t*>(0x40021000);
    uint32_t value = *rcc_cr;
    value = (value & ~(0x1F << 3)) | ((16 << 3) & (0x1F << 3));
    *rcc_cr = value;
}

// Test 7: Read multi-bit field
uint32_t test_template_read_field() {
    // Read HSI calibration value
    return cr::HSICAL::read(RCC->CR);
}

// Test 8: Manual read (reference)
uint32_t test_manual_read_field() {
    volatile uint32_t* rcc_cr = reinterpret_cast<volatile uint32_t*>(0x40021000);
    return (*rcc_cr & (0xFF << 8)) >> 8;
}

// Test 9: Clear bit
void test_template_clear_bit() {
    // Disable PLL
    RCC->CR = cr::PLLON::clear(RCC->CR);
}

// Test 10: Manual clear (reference)
void test_manual_clear_bit() {
    volatile uint32_t* rcc_cr = reinterpret_cast<volatile uint32_t*>(0x40021000);
    *rcc_cr &= ~(1 << 24);
}

// Test 11: Complex register configuration
void test_complex_config() {
    // Enable HSE, wait for ready, enable PLL
    RCC->CR = cr::HSEON::set(RCC->CR);
    while (!cr::HSERDY::test(RCC->CR)) { }
    RCC->CR = cr::PLLON::set(RCC->CR);
}

// Verification function (to prevent optimization)
extern "C" void verify_results(uint32_t a, uint32_t b, bool c, bool d) {
    (void)a; (void)b; (void)c; (void)d;
}

// Main test runner
int main() {
    // Run all tests
    test_template_set_bit();
    test_manual_set_bit();

    bool r1 = test_template_read_bit();
    bool r2 = test_manual_read_bit();

    test_template_write_field();
    test_manual_write_field();

    uint32_t v1 = test_template_read_field();
    uint32_t v2 = test_manual_read_field();

    test_template_clear_bit();
    test_manual_clear_bit();

    test_complex_config();

    // Verify results (prevents optimization)
    verify_results(v1, v2, r1, r2);

    return 0;
}
