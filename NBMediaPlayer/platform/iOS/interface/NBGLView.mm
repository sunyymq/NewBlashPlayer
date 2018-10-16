//
//  NBGLView.m
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 17/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import "NBGLView.h"
#import <OpenGLES/EAGL.h>
#import <QuartzCore/CAEAGLLayer.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#include <NBRendererInfo.h>

@interface NBGLView () {
    GLuint defaultFrameBuffer;
    GLuint colorRenderBuffer;
    GLint backingWidth;
    GLint backingHeight;
    EAGLContext* _glContext;
}

- (nb_status_t)prepareRendererCtx;
- (void)destroyRendererCtx;
- (nb_status_t)preRender:(NBRenderInfo*)info;
- (nb_status_t)postRender;

@end

@implementation NBGLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (BOOL)initGLContext {
    // Initialization code
    CAEAGLLayer* eaglLayer = (CAEAGLLayer*) self.layer;
    eaglLayer.opaque = TRUE;
    
    // set the draw code
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO],
                                    kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8,
                                    kEAGLDrawablePropertyColorFormat,nil];
    
    _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!_glContext || ![EAGLContext setCurrentContext:_glContext]) {
        return FALSE;
    }
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    
    glGenFramebuffers(1, &defaultFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
    
    glGenRenderbuffers(1, &colorRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
    
    [_glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              colorRenderBuffer);
    
    // Make videoScreen as output
    /// [Jack] This part should be separated from prepareTexture method because we dont need
    /// to re-bind output if only video source changes.
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return FALSE;
    }
    
    return TRUE;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    if (self = [super initWithCoder:aDecoder]) {
        if (![self initGLContext]) {
            return nil;
        }
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        if (![self initGLContext]) {
            return nil;
        }
    }
    return self;
}

- (void)dealloc {
    if (defaultFrameBuffer) {
        glDeleteFramebuffers(1, &defaultFrameBuffer);
        defaultFrameBuffer = 0;
    }
    
    if (colorRenderBuffer) {
        glDeleteRenderbuffers(1, &colorRenderBuffer);
        colorRenderBuffer = 0;
    }
    
    if ([EAGLContext currentContext] == _glContext) {
        [EAGLContext setCurrentContext:nil];
    }
    _glContext = nil;
}

- (nb_status_t)prepareRendererCtx {
    [EAGLContext setCurrentContext:_glContext];
    return OK;
}

- (void)destroyRendererCtx {
    if ([EAGLContext currentContext] == _glContext) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (nb_status_t)preRender:(NBRenderInfo*)info {
    // TODO: need to check if render is correctly settup
    [EAGLContext setCurrentContext:_glContext];
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);

    info->x = 0;
    info->y = 0;
    info->width = backingWidth;
    info->height = backingHeight;
    return OK;
}

- (nb_status_t)postRender {
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
    
//    const GLenum discards[]  = {GL_DEPTH_ATTACHMENT, GL_COLOR_ATTACHMENT0};
//    glDiscardFramebufferEXT(GL_FRAMEBUFFER, 2, discards);
    
    [_glContext presentRenderbuffer:GL_RENDERBUFFER];
    return OK;
}

- (void*)getRendererCtx {
    return (__bridge void*)_glContext;
}

@end

void* getRendererCtx(NBRendererTarget* target) {
    return [(__bridge NBGLView*)target->params getRendererCtx];
}

nb_status_t prepareRendererCtx(NBRendererTarget* target) {
    return [(__bridge NBGLView*)target->params prepareRendererCtx];
}

void destroyRendererCtx(NBRendererTarget* target) {
    return [(__bridge NBGLView*)target->params destroyRendererCtx];
}

nb_status_t preRender(NBRendererTarget* target, NBRenderInfo* info) {
    return [(__bridge NBGLView*)target->params preRender:info];
}

nb_status_t postRender(NBRendererTarget* target) {
    return [(__bridge NBGLView*)target->params postRender];
}
