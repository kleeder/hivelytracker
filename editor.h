#ifndef HIVELY_EDITOR_H
#define HIVELY_EDITOR_H

/* Each channel has note, instrument/waveform, FX1, parameter1, FX2, parameter2. */
#define EDITOR_FIELDS 6

int32 editor_column( const struct ahx_tune *at, BOOL perf );
int32 editor_row( const struct ahx_tune *at, BOOL perf );
void editor_focus( struct ahx_tune *at, BOOL perf, int32 col, int32 row );
struct editor_selection *editor_selection( struct ahx_tune *at, BOOL perf );
void editor_select( struct ahx_tune *at, BOOL perf, int32 col, int32 row, BOOL extend );
void editor_bounds( struct ahx_tune *at, BOOL perf, int32 *left, int32 *top, int32 *right, int32 *bottom );
BOOL editor_selected( struct ahx_tune *at, BOOL perf, int32 col, int32 row );
BOOL editor_copy( struct ahx_tune *at, BOOL perf, BOOL cut );
BOOL editor_paste( struct ahx_tune *at, BOOL perf );
BOOL editor_adjust( struct ahx_tune *at, BOOL perf, int32 delta, BOOL notes_only );
BOOL editor_clear( struct ahx_tune *at, BOOL perf );
BOOL editor_has_clipboard( BOOL perf );
void editor_forget_clipboard( BOOL perf );

#endif
