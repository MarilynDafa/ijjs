/* lws_config.h  Generated from lws_config.h.in  */

#ifndef NDEBUG
	#ifndef _DEBUG
		#define _DEBUG
	#endif
#endif

#define LWS_INSTALL_DATADIR ""
#define LWS_INSTALL_LIBDIR ""
#define LWS_LIBRARY_VERSION_MAJOR 4
#define LWS_LIBRARY_VERSION_MINOR 1
#define LWS_LIBRARY_VERSION_PATCH_ELABORATED 0-
#define LWS_LIBRARY_VERSION_PATCH 0

/* LWS_LIBRARY_VERSION_NUMBER looks like 1005001 for e.g. version 1.5.1 */
#define LWS_LIBRARY_VERSION_NUMBER (LWS_LIBRARY_VERSION_MAJOR * 1000000) + \
					(LWS_LIBRARY_VERSION_MINOR * 1000) + \
					LWS_LIBRARY_VERSION_PATCH
#define LWS_MAX_SMP 1

/* #undef LWS_ESP_PLATFORM */
/* #undef LWS_LIBRARY_VERSION_NUMBER */

/* #undef LWS_EXT_PTHREAD_LIBRARIES */

#define LWS_AVOID_SIGPIPE_IGN
#define LWS_BUILD_HASH
//#define LWS_BUILTIN_GETIFADDRS
#define LWS_CLIENT_HTTP_PROXYING
/* #undef LWS_DETECTED_PLAT_IOS */
#define LWS_FALLBACK_GETHOSTBYNAME
#define LWS_HAS_INTPTR_T
/* #undef LWS_HAS_GETOPT_LONG */
#define LWS_HAVE__ATOI64
#define LWS_HAVE_ATOLL
#define LWS_HAVE_BN_bn2binpad
/* #undef LWS_HAVE_CLOCK_GETTIME */
#define LWS_HAVE_EC_POINT_get_affine_coordinates
#define LWS_HAVE_ECDSA_SIG_set0
#define LWS_HAVE_EVP_MD_CTX_free
#define LWS_HAVE_EVP_aes_128_wrap
#define LWS_HAVE_EVP_aes_128_cfb8
#define LWS_HAVE_EVP_aes_128_cfb128
#define LWS_HAVE_EVP_aes_192_cfb8
#define LWS_HAVE_EVP_aes_192_cfb128
#define LWS_HAVE_EVP_aes_256_cfb8
#define LWS_HAVE_EVP_aes_256_cfb128
#define LWS_HAVE_EVP_aes_128_xts
#define LWS_HAVE_EVP_PKEY_new_raw_private_key
#define LWS_HAVE_EXECVPE
/* #undef LWS_HAVE_LIBCAP */
#define LWS_HAVE_HMAC_CTX_new
#define LWS_HAVE_MALLOC_H
/* #undef LWS_HAVE_MALLOC_TRIM */
/* #undef LWS_HAVE_MALLOC_USABLE_SIZE */
#define LWS_HAVE_mbedtls_md_setup 
#define LWS_HAVE_mbedtls_net_init 
#define LWS_HAVE_mbedtls_rsa_complete 
#define LWS_HAVE_mbedtls_internal_aes_encrypt 
#define LWS_HAVE_mbedtls_ssl_conf_alpn_protocols 
#define LWS_HAVE_mbedtls_ssl_get_alpn_protocol 
#define LWS_HAVE_mbedtls_ssl_conf_sni 
#define LWS_HAVE_mbedtls_ssl_set_hs_ca_chain 
#define LWS_HAVE_mbedtls_ssl_set_hs_own_cert 
#define LWS_HAVE_mbedtls_ssl_set_hs_authmode 
#define LWS_HAVE_MBEDTLS_NET_SOCKETS 
/* #undef LWS_HAVE_NEW_UV_VERSION_H */
/* #undef LWS_HAVE_OPENSSL_ECDH_H */
#define LWS_HAVE_PIPE2
#if !defined(WIN32) && !defined(_WIN32)
#define LWS_HAVE_EVENTFD
#endif
#define LWS_HAVE_PTHREAD_H
#define LWS_HAVE_RSA_SET0_KEY
/* #undef LWS_HAVE_RSA_verify_pss_mgf1 */
#define LWS_HAVE_SSL_CTX_get0_certificate
/* #undef LWS_HAVE_SSL_CTX_load_verify_file */
/* #undef LWS_HAVE_SSL_CTX_load_verify_dir */
#define LWS_HAVE_SSL_CTX_set1_param
#define LWS_HAVE_SSL_CTX_set_ciphersuites
#define LWS_HAVE_SSL_EXTRA_CHAIN_CERTS
#define LWS_HAVE_SSL_get0_alpn_selected
#define LWS_HAVE_SSL_CTX_EVP_PKEY_new_raw_private_key
#define LWS_HAVE_SSL_set_alpn_protos
#define LWS_HAVE_SSL_SET_INFO_CALLBACK
#define LWS_HAVE__STAT32I64
#define LWS_HAVE_STDINT_H
/* #undef LWS_HAVE_SYS_CAPABILITY_H */
#define LWS_HAVE_TLS_CLIENT_METHOD
#define LWS_HAVE_TLSV1_2_CLIENT_METHOD
/* #undef LWS_HAVE_UV_VERSION_H */
/* #undef LWS_HAVE_VFORK */
#define LWS_HAVE_X509_get_key_usage
#define LWS_HAVE_X509_VERIFY_PARAM_set1_host
#define LWS_LIBRARY_VERSION "4.1.0-"
#define LWS_LOGGING_BITFIELD_CLEAR 0
#define LWS_LOGGING_BITFIELD_SET 0
/* #undef LWS_MINGW_SUPPORT */
#define LWS_NO_CLIENT
#define LWS_NO_DAEMONIZE
/* #undef LWS_OPENSSL_CLIENT_CERTS */
#define LWS_OPENSSL_SUPPORT
/* #undef LWS_PLAT_OPTEE */
//#define LWS_PLAT_FREERTOS
#if !defined(WIN32) && !defined(_WIN32)
#define LWS_PLAT_UNIX
#endif
//#define LWS_ROLE_CGI
//#define LWS_ROLE_DBUS
#define LWS_ROLE_H1
#define LWS_ROLE_H2
#define LWS_ROLE_RAW
#define LWS_ROLE_RAW_FILE
#define LWS_ROLE_RAW_PROXY
#define LWS_ROLE_WS
#define LWS_ROLE_MQTT
/* #undef LWS_SHA1_USE_OPENSSL_NAME */
/* #undef LWS_SSL_CLIENT_USE_OS_CA_CERTS */
#define LWS_SSL_SERVER_WITH_ECDH_CERT
#define LWS_WITH_ABSTRACT
#define LWS_WITH_ACCESS_LOG
#define LWS_WITH_ACME
#define LWS_WITH_ALSA
#define LWS_WITH_SYS_ASYNC_DNS
/* #undef LWS_WITH_BORINGSSL */
//#define LWS_WITH_CGI
/* #undef LWS_WITH_CUSTOM_HEADERS */
/* #undef LWS_WITH_DEPRECATED_LWS_DLL */
#define LWS_WITH_DETAILED_LATENCY
/* #undef LWS_WITH_DIR */
#define LWS_WITH_DRIVERS
//#define LWS_WITH_ESP32
/* #undef LWS_HAVE_EVBACKEND_LINUXAIO */
/* #undef LWS_HAVE_EVBACKEND_IOURING */
/* #undef LWS_WITH_EXTERNAL_POLL */
#define LWS_WITH_FILE_OPS
#define LWS_WITH_FSMOUNT
/* #undef LWS_WITH_FTS */
#define LWS_WITH_GENCRYPTO
/* #undef LWS_WITH_GENERIC_SESSIONS */
/* #undef LWS_WITH_GLIB */
/* #undef LWS_WITH_GTK */
#define LWS_WITH_HTTP2
#define LWS_WITH_HTTP_BASIC_AUTH
//#define LWS_WITH_HTTP_BROTLI
#define LWS_HTTP_HEADERS_ALL
#define LWS_WITH_HTTP_PROXY
#define LWS_WITH_HTTP_STREAM_COMPRESSION
#define LWS_WITH_HTTP_UNCOMMON_HEADERS
#define LWS_WITH_IPV6
#define LWS_WITH_JOSE
#define LWS_WITH_LEJP
/* #undef LWS_WITH_LIBEV */
/* #undef LWS_WITH_LIBEVENT */
#define LWS_WITH_LIBUV
#define LWS_WITH_LWSAC
#define LWS_LOGS_TIMESTAMP
#define LWS_WITH_MBEDTLS
#define LWS_WITH_MINIZ
#define LWS_WITH_NETWORK
/* #undef LWS_WITH_NO_LOGS */
#define LWS_WITH_CLIENT
#define LWS_WITHOUT_EXTENSIONS
#define LWS_WITH_SERVER
#define LWS_WITH_SPAWN
#define LWS_WITH_PEER_LIMITS
/* #undef LWS_WITH_PLUGINS */
/* #undef LWS_WITH_POLARSSL */
#define LWS_WITH_POLL
#define LWS_WITH_RANGES
#define LWS_WITH_SECURE_STREAMS
#define LWS_WITH_SECURE_STREAMS_SYS_AUTH_API_AMAZON_COM
#define LWS_WITH_SECURE_STREAMS_PROXY_API
#define LWS_WITH_SECURE_STREAMS_STATIC_POLICY_ONLY
/* #undef LWS_WITH_SELFTESTS */
#define LWS_WITH_SEQUENCER
#define LWS_WITH_SERVER_STATUS
#define LWS_WITH_SYS_SMD
#define LWS_WITH_SMTP
#define LWS_WITH_SOCKS5
/* #undef LWS_WITH_STATEFUL_URLDECODE */
#define LWS_WITH_STATS
/* #undef LWS_WITH_STRUCT_SQLITE3 */
#define LWS_WITH_STRUCT_JSON
/* #undef LWS_WITH_SQLITE3 */
/* #undef LWS_WITH_SYS_NTPCLIENT */
/* #undef LWS_WITH_SYS_DHCP_CLIENT */
#define LWS_WITH_SYS_STATE
#define LWS_WITH_THREADPOOL
#define LWS_WITH_TLS
#define LWS_WITH_UDP
#define LWS_WITH_UNIX_SOCK
/* #undef LWS_WITH_ZIP_FOPS */
/* #undef USE_OLD_CYASSL */
/* #undef USE_WOLFSSL */
#define LWS_WITH_EVENT_LIBS
/* #undef LWS_WITH_EVLIB_PLUGINS */
#define LWS_WITH_LIBUV_INTERNAL
/* #undef LWS_WITH_PLUGINS_API */

