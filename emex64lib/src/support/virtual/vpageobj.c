/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <emex64lib/support/virtual/vpageobj.h>
#include <emex64lib/support/diag.h>

DEFINE_EVOBJECT_MAIN_EVENT_HANDLER(vpageobj)
{
    if(evarr == NULL)
    {
        return (int64_t)sizeof(vpageobj_t);
    }

    vpageobj_t *vo = (vpageobj_t*)evarr[0];

    switch(type)
    {
        case evObjEventCopy:
        case evObjEventSnapshot:
            diag_fatal(NULL, "vpageobj_t doesn't support being copied or snapshotted\n");
            exit(1);
        case evObjEventInit:
            vo->root = vpage_alloc();
            if(vo->root == NULL)
            {
                return -1;
            }

            return 0;
        case evObjEventDeinit:
            vpage_dealloc(vo->root);
            [[fallthrough]];
        default:
            return 0;
    }
}

void vpageobj_set_root(vpageobj_t *vo,
                       vpage_t *p,
                       vpage_t **old_out)
{
    if(old_out != NULL)
    {
        *old_out = vo->root;
    }
    else
    {
        vpage_dealloc(vo->root);
    }
    vo->root = p;
}
