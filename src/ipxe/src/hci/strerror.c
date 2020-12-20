#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ipxe/errortab.h>
#include <config/branding.h>

/** @file
 *
 * Error descriptions.
 *
 * The error numbers used by Etherboot are a superset of those defined
 * by the PXE specification version 2.1.  See errno.h for a listing of
 * the error values.
 *
 * To save space in ROM images, error string tables are optional.  Use
 * the ERRORMSG_XXX options in config.h to select which error string
 * tables you want to include.  If an error string table is omitted,
 * strerror() will simply return the text "Error 0x<errno>".
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/**
 * Retrieve string representation of error number.
 *
 * @v errno/rc		Error number or return status code
 * @ret strerror	Pointer to error text
 *
 * If the error is not found in the linked-in error tables, generates
 * a generic "Error 0x<errno>" message.
 *
 * The pointer returned by strerror() is valid only until the next
 * call to strerror().
 *
 */
char * strerror ( int errno ) {
	static char errbuf[64];

	errno         = 0; /* To get past compiler error (unused argument) */
 	errbuf[errno] = 0;

	return errbuf;
}

/* Do not include ERRFILE portion in the numbers in the error table */
#undef ERRFILE
#define ERRFILE 0

/** The most common errors */
struct errortab common_errors[] __errortab = {
	__einfo_errortab ( EINFO_ENOERR ),
	__einfo_errortab ( EINFO_EACCES ),
	__einfo_errortab ( EINFO_ECANCELED ),
	__einfo_errortab ( EINFO_ECONNRESET ),
	__einfo_errortab ( EINFO_EINVAL ),
	__einfo_errortab ( EINFO_EIO ),
	__einfo_errortab ( EINFO_ENETUNREACH ),
	__einfo_errortab ( EINFO_ENODEV ),
	__einfo_errortab ( EINFO_ENOENT ),
	__einfo_errortab ( EINFO_ENOEXEC ),
	__einfo_errortab ( EINFO_ENOMEM ),
	__einfo_errortab ( EINFO_ENOSPC ),
	__einfo_errortab ( EINFO_ENOTCONN ),
	__einfo_errortab ( EINFO_ENOTSUP ),
	__einfo_errortab ( EINFO_EPERM ),
	__einfo_errortab ( EINFO_ERANGE ),
	__einfo_errortab ( EINFO_ETIMEDOUT ),
};
