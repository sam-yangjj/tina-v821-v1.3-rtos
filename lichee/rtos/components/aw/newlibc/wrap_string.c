/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>

#define FLAGS_ZEROPAD (1U << 0U)
#define FLAGS_LEFT (1U << 1U)
#define FLAGS_PLUS (1U << 2U)
#define FLAGS_SPACE (1U << 3U)
#define FLAGS_HASH (1U << 4U)
#define FLAGS_UPPERCASE (1U << 5U)
#define FLAGS_CHAR (1U << 6U)
#define FLAGS_SHORT (1U << 7U)
#define FLAGS_LONG (1U << 8U)
#define FLAGS_LONG_LONG (1U << 9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

#ifndef PRINTF_NTOA_BUFFER_SIZE
#define PRINTF_NTOA_BUFFER_SIZE 32U
#endif

typedef void (*out_fct_type)(char character, void *buffer, size_t idx, size_t maxlen);

char *strcpy(char *dest, const char *src)
{
	char *ret = dest;
	while ((*dest++ = *src++) != 0)
	{
		;
	}
	return ret;
}

long unsigned int strlen(char *str)
{
	char *start = str;
	while (*str != '\0')
	{
		str++;
	}

	return str - start;
}

static inline void _out_null(char character, void *buffer, size_t idx, size_t maxlen)
{
	(void)character;
	(void)buffer;
	(void)idx;
	(void)maxlen;
}

// output the specified string in reverse, taking care of any zero-padding
static size_t _out_rev(out_fct_type out, char *buffer, size_t idx, size_t maxlen, const char *buf, size_t len, unsigned int width, unsigned int flags)
{
	const size_t start_idx = idx;

	// pad spaces up to given width
	if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD))
	{
		for (size_t i = len; i < width; i++)
		{
			out(' ', buffer, idx++, maxlen);
		}
	}

	// reverse string
	while (len)
	{
		out(buf[--len], buffer, idx++, maxlen);
	}

	// append pad spaces up to given width
	if (flags & FLAGS_LEFT)
	{
		while (idx - start_idx < width)
		{
			out(' ', buffer, idx++, maxlen);
		}
	}

	return idx;
}

// internal itoa format
static size_t _ntoa_format(out_fct_type out, char *buffer, size_t idx, size_t maxlen, char *buf, size_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
	// pad leading zeros
	if (!(flags & FLAGS_LEFT))
	{
		if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE))))
		{
			width--;
		}
		while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = '0';
		}
		while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = '0';
		}
	}

	// handle hash
	if (flags & FLAGS_HASH)
	{
		if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width)))
		{
			len--;
			if (len && (base == 16U))
			{
				len--;
			}
		}
		if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'x';
		}
		else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'X';
		}
		else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'b';
		}
		if (len < PRINTF_NTOA_BUFFER_SIZE)
		{
			buf[len++] = '0';
		}
	}

	if (len < PRINTF_NTOA_BUFFER_SIZE)
	{
		if (negative)
		{
			buf[len++] = '-';
		}
		else if (flags & FLAGS_PLUS)
		{
			buf[len++] = '+'; // ignore the space if the '+' exists
		}
		else if (flags & FLAGS_SPACE)
		{
			buf[len++] = ' ';
		}
	}

	return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline bool _is_digit(char ch)
{
	return (ch >= '0') && (ch <= '9');
}

// internal ASCII string to unsigned int conversion
static unsigned int _atoi(const char **str)
{
	unsigned int i = 0U;
	while (_is_digit(**str))
	{
		i = i * 10U + (unsigned int)(*((*str)++) - '0');
	}
	return i;
}

// internal itoa for 'long' type
static size_t _ntoa_long(out_fct_type out, char *buffer, size_t idx, size_t maxlen, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
	char buf[PRINTF_NTOA_BUFFER_SIZE];
	size_t len = 0U;

	// no hash for 0 values
	if (!value)
	{
		flags &= ~FLAGS_HASH;
	}

	// write if precision != 0 and value is != 0
	if (!(flags & FLAGS_PRECISION) || value)
	{
		do
		{
			const char digit = (char)(value % base);
			buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
			value /= base;
		} while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
	}

	return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}

// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
static inline unsigned int _strnlen_s(const char *str, size_t maxsize)
{
	const char *s;
	for (s = str; s && *s && (maxsize-- >= 0); ++s)
		;
	return (unsigned int)(s - str);
}

// internal vsnprintf
static int _vsnprintf(out_fct_type out, char *buffer, const size_t maxlen, const char *format, va_list va)
{
	unsigned int flags, width, precision, n;
	size_t idx = 0U;

	if (!buffer)
	{
		// use null output function
		out = _out_null;
	}

	while (*format)
	{
		// format specifier?  %[flags][width][.precision][length]
		if (*format != '%')
		{
			// no
			out(*format, buffer, idx++, maxlen);
			format++;
			continue;
		}
		else
		{
			// yes, evaluate it
			format++;
		}

		// evaluate flags
		flags = 0U;
		do
		{
			switch (*format)
			{
			case '0':
				flags |= FLAGS_ZEROPAD;
				format++;
				n = 1U;
				break;
			case '-':
				flags |= FLAGS_LEFT;
				format++;
				n = 1U;
				break;
			case '+':
				flags |= FLAGS_PLUS;
				format++;
				n = 1U;
				break;
			case ' ':
				flags |= FLAGS_SPACE;
				format++;
				n = 1U;
				break;
			case '#':
				flags |= FLAGS_HASH;
				format++;
				n = 1U;
				break;
			default:
				n = 0U;
				break;
			}
		} while (n);

		// evaluate width field
		width = 0U;
		if (_is_digit(*format))
		{
			width = _atoi(&format);
		}
		else if (*format == '*')
		{
			const int w = va_arg(va, int);
			if (w < 0)
			{
				flags |= FLAGS_LEFT; // reverse padding
				width = (unsigned int)-w;
			}
			else
			{
				width = (unsigned int)w;
			}
			format++;
		}

		// evaluate precision field
		precision = 0U;
		if (*format == '.')
		{
			flags |= FLAGS_PRECISION;
			format++;
			if (_is_digit(*format))
			{
				precision = _atoi(&format);
			}
			else if (*format == '*')
			{
				const int prec = (int)va_arg(va, int);
				precision = prec > 0 ? (unsigned int)prec : 0U;
				format++;
			}
		}

		// evaluate length field
		switch (*format)
		{
		case 'l':
			flags |= FLAGS_LONG;
			format++;
			if (*format == 'l')
			{
				flags |= FLAGS_LONG_LONG;
				format++;
			}
			break;
		case 'h':
			flags |= FLAGS_SHORT;
			format++;
			if (*format == 'h')
			{
				flags |= FLAGS_CHAR;
				format++;
			}
			break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
		case 't':
			flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
			format++;
			break;
#endif
		case 'j':
			flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
			format++;
			break;
		case 'z':
			flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
			format++;
			break;
		default:
			break;
		}

		// evaluate specifier
		switch (*format)
		{
		case 'd':
		case 'i':
		case 'u':
		case 'x':
		case 'X':
		case 'o':
		case 'b':
		{
			// set the base
			unsigned int base;
			if (*format == 'x' || *format == 'X')
			{
				base = 16U;
			}
			else if (*format == 'o')
			{
				base = 8U;
			}
			else if (*format == 'b')
			{
				base = 2U;
			}
			else
			{
				base = 10U;
				flags &= ~FLAGS_HASH; // no hash for dec format
			}
			// uppercase
			if (*format == 'X')
			{
				flags |= FLAGS_UPPERCASE;
			}

			// no plus or space flag for u, x, X, o, b
			if ((*format != 'i') && (*format != 'd'))
			{
				flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
			}

			// ignore '0' flag when precision is given
			if (flags & FLAGS_PRECISION)
			{
				flags &= ~FLAGS_ZEROPAD;
			}

			// convert the integer
			if ((*format == 'i') || (*format == 'd'))
			{
				// signed
				if (flags & FLAGS_LONG_LONG)
				{
#if defined(PRINTF_SUPPORT_LONG_LONG)
					const long long value = va_arg(va, long long);
					idx = _ntoa_long_long(out, buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
#endif
				}
				else if (flags & FLAGS_LONG)
				{
					const long value = va_arg(va, long);
					idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
				}
				else
				{
					const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int)
																										   : va_arg(va, int);
					idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
				}
			}
			else
			{
				// unsigned
				if (flags & FLAGS_LONG_LONG)
				{
#if defined(PRINTF_SUPPORT_LONG_LONG)
					idx = _ntoa_long_long(out, buffer, idx, maxlen, va_arg(va, unsigned long long), false, base, precision, width, flags);
#endif
				}
				else if (flags & FLAGS_LONG)
				{
					idx = _ntoa_long(out, buffer, idx, maxlen, va_arg(va, unsigned long), false, base, precision, width, flags);
				}
				else
				{
					const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int)
																																	  : va_arg(va, unsigned int);
					idx = _ntoa_long(out, buffer, idx, maxlen, value, false, base, precision, width, flags);
				}
			}
			format++;
			break;
		}
#if defined(PRINTF_SUPPORT_FLOAT)
		case 'f':
		case 'F':
			if (*format == 'F')
				flags |= FLAGS_UPPERCASE;
			idx = _ftoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
			format++;
			break;
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
		case 'e':
		case 'E':
		case 'g':
		case 'G':
			if ((*format == 'g') || (*format == 'G'))
				flags |= FLAGS_ADAPT_EXP;
			if ((*format == 'E') || (*format == 'G'))
				flags |= FLAGS_UPPERCASE;
			idx = _etoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
			format++;
			break;
#endif // PRINTF_SUPPORT_EXPONENTIAL
#endif // PRINTF_SUPPORT_FLOAT
		case 'c':
		{
			unsigned int l = 1U;
			// pre padding
			if (!(flags & FLAGS_LEFT))
			{
				while (l++ < width)
				{
					out(' ', buffer, idx++, maxlen);
				}
			}
			// char output
			out((char)va_arg(va, int), buffer, idx++, maxlen);
			// post padding
			if (flags & FLAGS_LEFT)
			{
				while (l++ < width)
				{
					out(' ', buffer, idx++, maxlen);
				}
			}
			format++;
			break;
		}

		case 's':
		{
			const char *p = va_arg(va, char *);
			unsigned int l = _strnlen_s(p, precision ? precision : (size_t)-1);
			// pre padding
			if (flags & FLAGS_PRECISION)
			{
				l = (l < precision ? l : precision);
			}
			if (!(flags & FLAGS_LEFT))
			{
				while (l++ < width)
				{
					out(' ', buffer, idx++, maxlen);
				}
			}
			// string output
			while (p && (*p != 0) && (!(flags & FLAGS_PRECISION) || precision--))
			{
				out(*(p++), buffer, idx++, maxlen);
			}
			// post padding
			if (flags & FLAGS_LEFT)
			{
				while (l++ < width)
				{
					out(' ', buffer, idx++, maxlen);
				}
			}
			format++;
			break;
		}

		case 'p':
		{
			width = sizeof(void *) * 2U;
			flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
			const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
			if (is_ll)
			{
				idx = _ntoa_long_long(out, buffer, idx, maxlen, (uintptr_t)va_arg(va, void *), false, 16U, precision, width, flags);
			}
			else
			{
#endif
				idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)((uintptr_t)va_arg(va, void *)), false, 16U, precision, width, flags);
#if defined(PRINTF_SUPPORT_LONG_LONG)
			}
#endif
			format++;
			break;
		}

		case '%':
			out('%', buffer, idx++, maxlen);
			format++;
			break;

		default:
			out(*format, buffer, idx++, maxlen);
			format++;
			break;
		}
	}

	// termination
	out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

	// return written chars without terminating \0
	return (int)idx;
}

// internal buffer output
static inline void _out_buffer(char character, void *buffer, size_t idx, size_t maxlen)
{
	if (idx < maxlen)
	{
		((char *)buffer)[idx] = character;
	}
}

int sprintf(char *buffer, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, format, va);
	va_end(va);

	return ret;
}
