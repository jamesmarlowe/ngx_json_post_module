/*!
 * \file ngx_json_post_module.h
 * \author James Marlowe https://github.com/jamesmarlowe/
 * \brief Nginx module that reads HTTP POST into variables from json encoded body
 */

#include <string.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_flag_t ngx_json_post_used = 0;

typedef struct {
    ngx_flag_t    enabled;
} ngx_json_post_loc_conf_t;

typedef struct 
{
  unsigned          done:1;
  unsigned          waiting_more_body:1;
} ngx_json_post_ctx_t;

static ngx_int_t ngx_json_post_init(ngx_conf_t *cf);
static ngx_int_t ngx_json_post_handler(ngx_http_request_t *r);
static void ngx_json_post_read(ngx_http_request_t *r);
static char *ngx_json_post_conf_handler(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
    
static ngx_command_t ngx_json_post_commands[] = {

    { ngx_string("json_decode"),
      NGX_HTTP_LOC_CONF,
      ngx_json_post_conf_handler,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};

static ngx_http_module_t ngx_json_post_module_ctx = {
    NULL,                                   /* preconfiguration */
    ngx_json_post_init,                     /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};

ngx_module_t  ngx_json_post_module = {
    NGX_MODULE_V1,
    &ngx_json_post_module_ctx,        /* module context */
    ngx_json_post_commands,           /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

/* register a new rewrite phase handler */
static ngx_int_t
ngx_json_post_init(ngx_conf_t *cf)
{

    ngx_http_handler_pt             *h;
    ngx_http_core_main_conf_t       *cmcf;

    if (!ngx_json_post_used) {
        return NGX_OK;
    }
    
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);

    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_json_post_handler;

    return NGX_OK;
}

static ngx_int_t
ngx_json_post_handler(ngx_http_request_t *r)
{
    ngx_json_post_ctx_t       *ctx;
    ngx_int_t                  rc;

    ngx_log_error(NGX_LOG_DEBUG , r->connection->log, 0,
                   "ngx_json_post rewrite phase handler");
    
    ctx = ngx_http_get_module_ctx(r, ngx_json_post_module);
    
    if (ctx != NULL) {
        if (ctx->done) {
            ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                           "ngx_json_post rewrite phase handler done");

            return NGX_DECLINED;
        }

        return NGX_DONE;
    }
    
    if (r->method != NGX_HTTP_POST) {
        return NGX_DECLINED;
    }
    
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_json_post_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }
    
    ngx_http_set_ctx(r, ctx, ngx_json_post_module);
    
    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                   "ngx_json_post start to read client request body");
    
    rc = ngx_http_read_client_request_body(r, ngx_json_post_read);
    
    if (rc == NGX_ERROR || rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    
    if (rc == NGX_AGAIN) {
        ctx->waiting_more_body = 1;

        return NGX_DONE;
    }

    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                   "ngx_json_post has read the request body in one run");

    return NGX_DECLINED;
    
}

static void ngx_json_post_read(ngx_http_request_t *r)
{
    ngx_json_post_ctx_t     *ctx;

    ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0,
                   "http form_input post read request body");

    ctx = ngx_http_get_module_ctx(r, ngx_json_post_module);

    ctx->done = 1;

#if defined(nginx_version) && nginx_version >= 8011
    dd("count--");
    r->main->count--;
#endif

    /* waiting_more_body my rewrite phase handler */
    if (ctx->waiting_more_body) {
        ctx->waiting_more_body = 0;

        ngx_http_core_run_phases(r);
    }
}

static char *ngx_json_post_conf_handler(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf)
{
    ngx_json_post_used = 1;
    return NGX_CONF_OK;
}
