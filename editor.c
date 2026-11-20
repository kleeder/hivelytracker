/* Field selections and typed rectangular clipboards for both editors. */
#include <string.h>
#include <system_includes.h>
#include "replay.h"
#include "gui.h"
#include "undo.h"
#include "editor.h"

struct editor_clipboard
{
  int32 width, height, first;
  uint16 cells[64*MAX_CHANNELS*EDITOR_FIELDS];
};

static struct editor_clipboard clipboards[2];
static const int32 pattern_fields[] = { 0, 1, 1, 2, 3, 3, 4, 5, 5 };
static const int32 perf_fields[] = { 0, 1, 2, 3, 3, 4, 5, 5 };
static const int32 pattern_cursors[] = { 0, 1, 3, 4, 6, 7 };
static const int32 perf_cursors[] = { 0, 1, 2, 3, 5, 6 };

static int32 limit( int32 value, int32 min, int32 max )
{
  if( value < min ) return min;
  if( value > max ) return max;
  return value;
}

static int32 columns( const struct ahx_tune *at, BOOL perf )
{
  return perf ? EDITOR_FIELDS : at->at_Channels*EDITOR_FIELDS;
}

static int32 rows( const struct ahx_tune *at, BOOL perf )
{
  return perf ? 255 : at->at_TrackLength;
}

int32 editor_column( const struct ahx_tune *at, BOOL perf )
{
  if( perf ) return perf_fields[at->at_Instruments[at->at_curins].ins_pcurx];
  return (at->at_curlch+at->at_tracked_curs/9)*EDITOR_FIELDS+
         pattern_fields[at->at_tracked_curs%9];
}

int32 editor_row( const struct ahx_tune *at, BOOL perf )
{
  return perf ? at->at_Instruments[at->at_curins].ins_pcury : at->at_NoteNr;
}

void editor_focus( struct ahx_tune *at, BOOL perf, int32 col, int32 row )
{
  int32 channel;
  col = limit( col, 0, columns(at, perf)-1 );
  row = limit( row, 0, rows(at, perf)-1 );
  if( perf )
  {
    at->at_Instruments[at->at_curins].ins_pcurx = perf_cursors[col];
    at->at_Instruments[at->at_curins].ins_pcury = row;
  } else {
    channel = col/EDITOR_FIELDS;
    if( channel < at->at_curlch ) at->at_curlch = channel;
    if( channel >= at->at_curlch+6 ) at->at_curlch = channel-5;
    at->at_tracked_curs = (channel-at->at_curlch)*9+pattern_cursors[col%EDITOR_FIELDS];
    at->at_NoteNr = row;
    at->at_editing = E_TRACK;
  }
}

struct editor_selection *editor_selection( struct ahx_tune *at, BOOL perf )
{
  struct editor_selection *sel = perf ? &at->at_perf_selection : &at->at_pattern_selection;
  int32 context = perf ? at->at_curins : at->at_PosNr;
  if( sel->active && (sel->context != context || sel->col >= columns(at, perf) ||
      sel->anchor_col >= columns(at, perf) || sel->row >= rows(at, perf) ||
      sel->anchor_row >= rows(at, perf)) ) sel->active = FALSE;
  return sel;
}

void editor_select( struct ahx_tune *at, BOOL perf, int32 col, int32 row, BOOL extend )
{
  struct editor_selection *sel = editor_selection( at, perf );
  if( !extend || !sel->active )
  {
    sel->anchor_col = extend ? editor_column(at, perf) : limit(col, 0, columns(at, perf)-1);
    sel->anchor_row = extend ? editor_row(at, perf) : limit(row, 0, rows(at, perf)-1);
  }
  editor_focus( at, perf, col, row );
  sel->col = editor_column(at, perf);
  sel->row = editor_row(at, perf);
  sel->context = perf ? at->at_curins : at->at_PosNr;
  sel->active = TRUE;
  if( !perf ) at->at_cbmarktrack = -1;
}

void editor_bounds( struct ahx_tune *at, BOOL perf, int32 *left, int32 *top, int32 *right, int32 *bottom )
{
  struct editor_selection *sel = editor_selection( at, perf );
  *left = *right = editor_column(at, perf);
  *top = *bottom = editor_row(at, perf);
  if( !sel->active ) return;
  *left = sel->anchor_col < sel->col ? sel->anchor_col : sel->col;
  *right = sel->anchor_col > sel->col ? sel->anchor_col : sel->col;
  *top = sel->anchor_row < sel->row ? sel->anchor_row : sel->row;
  *bottom = sel->anchor_row > sel->row ? sel->anchor_row : sel->row;
}

BOOL editor_selected( struct ahx_tune *at, BOOL perf, int32 col, int32 row )
{
  int32 left, top, right, bottom;
  if( !editor_selection(at, perf)->active ) return FALSE;
  editor_bounds( at, perf, &left, &top, &right, &bottom );
  return col >= left && col <= right && row >= top && row <= bottom;
}

static BOOL editable( const struct ahx_tune *at, BOOL perf )
{
  if( at->at_doing != D_IDLE && at->at_doing != D_EDITING ) return FALSE;
  return !perf || at->at_curins > 0;
}

static uint16 get_cell( struct ahx_tune *at, BOOL perf, int32 col, int32 row )
{
  struct ahx_step *step;
  struct ahx_plsentry *entry;
  if( perf )
  {
    entry = &at->at_Instruments[at->at_curins].ins_PList.pls_Entries[row];
    switch( col )
    {
      case 0: return entry->ple_Note | (entry->ple_Fixed ? 256 : 0);
      case 1: return entry->ple_Waveform;
      case 2: return entry->ple_FX[0] & 15;
      case 3: return entry->ple_FXParam[0] & 255;
      case 4: return entry->ple_FX[1] & 15;
      default: return entry->ple_FXParam[1] & 255;
    }
  }
  step = &at->at_Tracks[at->at_Positions[at->at_PosNr].pos_Track[col/EDITOR_FIELDS]][row];
  switch( col%EDITOR_FIELDS )
  {
    case 0: return step->stp_Note;
    case 1: return step->stp_Instrument;
    case 2: return step->stp_FX;
    case 3: return step->stp_FXParam;
    case 4: return step->stp_FXb;
    default: return step->stp_FXbParam;
  }
}

static void put_cell( struct ahx_tune *at, BOOL perf, int32 col, int32 row, uint16 value )
{
  struct ahx_step *step;
  struct ahx_plsentry *entry;
  if( perf )
  {
    entry = &at->at_Instruments[at->at_curins].ins_PList.pls_Entries[row];
    switch( col )
    {
      case 0: entry->ple_Note = value & 255; entry->ple_Fixed = (value & 256) != 0; break;
      case 1: entry->ple_Waveform = value; break;
      case 2: entry->ple_FX[0] = value; break;
      case 3: entry->ple_FXParam[0] = value; break;
      case 4: entry->ple_FX[1] = value; break;
      case 5: entry->ple_FXParam[1] = value; break;
    }
    return;
  }
  step = &at->at_Tracks[at->at_Positions[at->at_PosNr].pos_Track[col/EDITOR_FIELDS]][row];
  switch( col%EDITOR_FIELDS )
  {
    case 0: step->stp_Note = value; break;
    case 1: step->stp_Instrument = value; break;
    case 2: step->stp_FX = value; break;
    case 3: step->stp_FXParam = value; break;
    case 4: step->stp_FXb = value; break;
    case 5: step->stp_FXbParam = value; break;
  }
}

static struct undonode *begin_edit( struct ahx_tune *at, BOOL perf, int32 left, int32 right )
{
  if( !perf ) return begin_track_region( at, left/EDITOR_FIELDS, right/EDITOR_FIELDS );
  setbefore_plist( at, at->at_Instruments[at->at_curins].ins_PList.pls_Entries );
  return NULL;
}

static BOOL end_edit( struct ahx_tune *at, BOOL perf, struct undonode *un )
{
  struct ahx_plsentry *entries;
  if( !perf ) return end_track_region( at, un );
  entries = at->at_Instruments[at->at_curins].ins_PList.pls_Entries;
  if( !memcmp(entries, at->at_rem_pls_Entries, sizeof(at->at_rem_pls_Entries)) ) return FALSE;
  setafter_plist( at, entries );
  at->at_modified = TRUE;
  return TRUE;
}

BOOL editor_clear( struct ahx_tune *at, BOOL perf )
{
  int32 left, top, right, bottom, x, y;
  struct undonode *un;
  if( !editable(at, perf) ) return FALSE;
  editor_bounds( at, perf, &left, &top, &right, &bottom );
  un = begin_edit( at, perf, left, right );
  if( !perf && !un ) return FALSE;
  for( y=top; y<=bottom; y++ )
    for( x=left; x<=right; x++ ) put_cell( at, perf, x, y, 0 );
  return end_edit( at, perf, un );
}

BOOL editor_copy( struct ahx_tune *at, BOOL perf, BOOL cut )
{
  struct editor_clipboard *cb = &clipboards[perf ? 1 : 0];
  int32 left, top, right, bottom, x, y, i = 0;
  if( cut && !editable(at, perf) ) return FALSE;
  editor_bounds( at, perf, &left, &top, &right, &bottom );
  cb->width = right-left+1;
  cb->height = bottom-top+1;
  cb->first = left%EDITOR_FIELDS;
  for( y=top; y<=bottom; y++ )
    for( x=left; x<=right; x++ ) cb->cells[i++] = get_cell( at, perf, x, y );
  if( cut ) editor_clear( at, perf );
  return TRUE;
}

BOOL editor_has_clipboard( BOOL perf )
{
  return clipboards[perf ? 1 : 0].width > 0;
}

void editor_forget_clipboard( BOOL perf )
{
  clipboards[perf ? 1 : 0].width = 0;
}

static int32 cell_type( int32 col )
{
  col %= EDITOR_FIELDS;
  return col >= 4 ? col-2 : col;  /* Both effect slots are compatible. */
}

BOOL editor_paste( struct ahx_tune *at, BOOL perf )
{
  struct editor_clipboard *cb = &clipboards[perf ? 1 : 0];
  struct undonode *un;
  int32 left, top, right, bottom, x, y;
  if( !cb->width || !editable(at, perf) ) return FALSE;
  left = editor_column(at, perf);
  top = editor_row(at, perf);
  right = limit(left+cb->width-1, 0, columns(at, perf)-1);
  bottom = limit(top+cb->height-1, 0, rows(at, perf)-1);
  /* Reject incompatible starting fields instead of silently losing cells. */
  if( cell_type(left) != cell_type(cb->first) ) return FALSE;
  un = begin_edit( at, perf, left, right );
  if( !perf && !un ) return FALSE;
  for( y=top; y<=bottom; y++ )
    for( x=left; x<=right; x++ )
      if( cell_type(x) == cell_type(cb->first+x-left) )
        put_cell( at, perf, x, y, cb->cells[(y-top)*cb->width+x-left] );
  return end_edit( at, perf, un );
}

BOOL editor_adjust( struct ahx_tune *at, BOOL perf, int32 delta, BOOL notes_only )
{
  struct undonode *un;
  int32 left, top, right, bottom, x, y, field, value, next, max, channel, track, i;
  uint8 seen[256] = { 0 };
  uint16 original;
  if( !editable(at, perf) || !delta ) return FALSE;
  editor_bounds( at, perf, &left, &top, &right, &bottom );
  if( notes_only && !editor_selection(at, perf)->active )
    left = right = (left/EDITOR_FIELDS)*EDITOR_FIELDS;
  un = begin_edit( at, perf, left, right );
  if( !perf && !un ) return FALSE;
  for( x=left; x<=right; x++ )
  {
    field = x%EDITOR_FIELDS;
    if( notes_only && field != 0 ) continue;
    if( !perf )
    {
      channel = x/EDITOR_FIELDS;
      track = at->at_Positions[at->at_PosNr].pos_Track[channel];
      /* A shared track must only be transposed once per selected field. */
      if( seen[track] & (1<<field) ) continue;
      seen[track] |= 1<<field;
    }
    for( y=top; y<=bottom; y++ )
    {
      original = get_cell( at, perf, x, y );
      value = field == 0 ? original & 255 : original;
      if( field == 0 && !value ) continue; /* Empty notes stay empty. */
      max = field == 0 ? 60 : field == 1 ? (perf ? 4 : 63) : (field%2 == 0 ? 15 : 255);
      next = limit( value+delta, field == 0 ? 1 : 0, max );
      i = (field == 0 ? original & 256 : 0) | next;
      put_cell( at, perf, x, y, i );
    }
  }
  return end_edit( at, perf, un );
}
