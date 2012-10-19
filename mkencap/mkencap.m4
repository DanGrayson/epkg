m4_divert(-1)

m4_changequote(<|,|>)

m4_define(<|_PLATFORM_IF_EQUAL|>, <|m4_define(<|__IF_BLOCK|>)m4_ifelse(PLATFORM, <|$1|>, <|m4_divert(0)m4_define(<|__IF_FOUND|>)|>, <|m4_divert(-1)|>)|>)

m4_define(<|PLATFORM_IF_EQUAL|>, <|m4_ifdef(<|__IF_BLOCK|>, <|m4_divert(0)m4_errprint(<|m4: |>m4___file__:m4___line__<|: cannot nest IF blocks
|>)m4_m4exit(1)|>)_PLATFORM_IF_EQUAL($@)m4_dnl|>)

m4_define(<|_PLATFORM_IF_MATCH|>, <|m4_define(<|__IF_BLOCK|>)m4_ifelse(m4_regexp(PLATFORM, <|$1|>), -1, <|m4_divert(-1)|>, <|m4_divert(0)m4_define(<|__IF_FOUND|>)|>)|>)

m4_define(<|PLATFORM_IF_MATCH|>, <|m4_ifdef(<|__IF_BLOCK|>, <|m4_divert(0)m4_errprint(<|m4: |>m4___file__:m4___line__<|: cannot nest IF blocks
|>)m4_m4exit(1)|>)_PLATFORM_IF_MATCH($@)m4_dnl|>)

m4_define(<|PLATFORM_ELSE_IF_EQUAL|>, <|m4_ifdef(<|__IF_FOUND|>, <|m4_divert(-1)|>, <|_PLATFORM_IF_EQUAL($@)|>)m4_dnl|>)

m4_define(<|PLATFORM_ELSE_IF_MATCH|>, <|m4_ifdef(<|__IF_FOUND|>, <|m4_divert(-1)|>, <|_PLATFORM_IF_MATCH($@)|>)m4_dnl|>)

m4_define(<|PLATFORM_ELSE|>, <|m4_ifdef(<|__IF_FOUND|>, <|m4_divert(-1)|>, <|m4_divert(0)|>)m4_dnl|>)

m4_define(<|PLATFORM_ENDIF|>, <|m4_divert(0)m4_undefine(<|__IF_FOUND|>)m4_undefine(<|__IF_BLOCK|>)m4_dnl|>)

m4_m4wrap(<|m4_ifdef(<|__IF_BLOCK|>, <|m4_errprint(<|m4: unexpected EOF (missing PLATFORM_ENDIF ?)
|>)m4_m4exit(1)|>)|>)

m4_divert(0)m4_dnl
