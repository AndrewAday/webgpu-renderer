#include "entity.h"

#include <glm/gtx/quaternion.hpp> // quatToMat4

void Entity::init(Entity* entity)
{
    // zero out
    *entity = {};
    // init transform
    entity->pos = glm::vec3(0.0);
    entity->rot = IDENTITY_ROT;
    entity->sca = glm::vec3(1.0);

    // init camera params
    entity->fovDegrees = 45.0f;
    entity->aspect     = 1.0f; // TODO get from window in g_ctx
    entity->nearPlane  = 0.1f;
    entity->farPlane   = 1000.0f;
}

glm::mat4 Entity::modelMatrix(Entity* entity)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, entity->pos);
    M           = M * glm::toMat4(entity->rot);
    M           = glm::scale(M, entity->sca);
    return M;
}

glm::mat4 Entity::viewMatrix(Entity* entity)
{
    return glm::inverse(modelMatrix(entity));
}

glm::mat4 Entity::projectionMatrix(Entity* entity)
{
    return glm::perspective(glm::radians(entity->fovDegrees), entity->aspect,
                            entity->nearPlane, entity->farPlane);
}

void Entity::rotateOnLocalAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = entity->rot * glm::angleAxis(deg, glm::normalize(axis));
}

void Entity::rotateOnWorldAxis(Entity* entity, glm::vec3 axis, f32 deg)
{
    entity->rot = glm::angleAxis(deg, glm::normalize(axis)) * entity->rot;
}

// ============================================================================
// Orbit Controls
// ============================================================================

// OrbitControls performs orbiting, dollying (zooming), and panning.
// Unlike TrackballControls, it maintains the "up" direction object.up (+Y by
// default).
//
//    Orbit - left mouse / touch: one-finger move
//    Zoom - middle mouse, or mousewheel / touch: two-finger spread or squish
//    Pan - right mouse, or left mouse + ctrl/meta/shiftKey, or arrow keys /
//    touch: two-finger move

// const _ray         = new Ray();
// const _plane       = new Plane();
// const TILT_LIMIT   = Math.cos(70 * MathUtils.DEG2RAD);

// class OrbitControls extends EventDispatcher
// {

//     constructor(object)
//     {
//         // Mouse buttons
//         this.mouseButtons
//           = { LEFT : MOUSE.ROTATE, MIDDLE : MOUSE.DOLLY, RIGHT : MOUSE.PAN };

//         this.getPolarAngle = function()
//         {
//             return spherical.phi;
//         };

//         this.getAzimuthalAngle = function()
//         {
//             return spherical.theta;
//         };

//         this.getDistance = function()
//         {
//             return this.object.position.distanceTo(this.target);
//         };

//         // this method is exposed, but perhaps it would be better if we can
//         make
//         // it private...
//         this.update = function()
//         {
//             const offset = new Vector3();

//             // so camera.up is the orbit axis
//             const quat = new Quaternion().setFromUnitVectors(
//               object.up, new Vector3(0, 1, 0));
//             const quatInverse = quat.clone().invert();

//             const lastPosition       = new Vector3();
//             const lastQuaternion     = new Quaternion();
//             const lastTargetPosition = new Vector3();

//             return function update(deltaTime = null)
//             {

//                 const position = scope.object.position;

//                 offset.copy(position).sub(scope.target);

//                 // rotate offset to "y-axis-is-up" space
//                 offset.applyQuaternion(quat);

//                 // angle from z-axis around y-axis
//                 spherical.setFromVector3(offset);

//                 if (scope.enableDamping) {

//                     spherical.theta
//                       += sphericalDelta.theta * scope.dampingFactor;
//                     spherical.phi += sphericalDelta.phi *
//                     scope.dampingFactor;

//                 } else {

//                     spherical.theta += sphericalDelta.theta;
//                     spherical.phi += sphericalDelta.phi;
//                 }

//                 // restrict theta to be between desired limits

//                 let min = scope.minAzimuthAngle;
//                 let max = scope.maxAzimuthAngle;

//                 if (isFinite(min) && isFinite(max)) {

//                     if (min < -Math.PI)
//                         min += twoPI;
//                     else if (min > Math.PI)
//                         min -= twoPI;

//                     if (max < -Math.PI)
//                         max += twoPI;
//                     else if (max > Math.PI)
//                         max -= twoPI;

//                     if (min <= max) {

//                         spherical.theta
//                           = Math.max(min, Math.min(max, spherical.theta));

//                     } else {

//                         spherical.theta = (spherical.theta > (min + max) / 2)
//                         ?
//                                             Math.max(min, spherical.theta) :
//                                             Math.min(max, spherical.theta);
//                     }
//                 }

//                 // restrict phi to be between desired limits
//                 spherical.phi
//                   = Math.max(scope.minPolarAngle,
//                              Math.min(scope.maxPolarAngle, spherical.phi));

//                 spherical.makeSafe();

//                 // move target to panned location

//                 if (scope.enableDamping == = true) {

//                     scope.target.addScaledVector(panOffset,
//                                                  scope.dampingFactor);

//                 } else {

//                     scope.target.add(panOffset);
//                 }

//                 // Limit the target distance from the cursor to create a
//                 sphere
//                 // around the center of interest
//                 scope.target.sub(scope.cursor);
//                 scope.target.clampLength(scope.minTargetRadius,
//                                          scope.maxTargetRadius);
//                 scope.target.add(scope.cursor);

//                 let zoomChanged = false;
//                 // adjust the camera position based on zoom only if we're not
//                 // zooming to the cursor or if it's an ortho camera we adjust
//                 // zoom later in these cases
//                 if (scope.zoomToCursor && performCursorZoom
//                     || scope.object.isOrthographicCamera) {

//                     spherical.radius = clampDistance(spherical.radius);

//                 } else {

//                     const prevRadius = spherical.radius;
//                     spherical.radius = clampDistance(spherical.radius *
//                     scale); zoomChanged      = prevRadius !=
//                     spherical.radius;
//                 }

//                 offset.setFromSpherical(spherical);

//                 // rotate offset back to "camera-up-vector-is-up" space
//                 offset.applyQuaternion(quatInverse);

//                 position.copy(scope.target).add(offset);

//                 scope.object.lookAt(scope.target);

//                 if (scope.enableDamping == = true) {

//                     sphericalDelta.theta *= (1 - scope.dampingFactor);
//                     sphericalDelta.phi *= (1 - scope.dampingFactor);

//                     panOffset.multiplyScalar(1 - scope.dampingFactor);

//                 } else {

//                     sphericalDelta.set(0, 0, 0);

//                     panOffset.set(0, 0, 0);
//                 }

//                 // adjust camera position
//                 if (scope.zoomToCursor && performCursorZoom) {

//                     let newRadius = null;
//                     if (scope.object.isPerspectiveCamera) {

//                         // move the camera down the pointer ray
//                         // this method avoids floating point error
//                         const prevRadius = offset.length();
//                         newRadius        = clampDistance(prevRadius * scale);

//                         const radiusDelta = prevRadius - newRadius;
//                         scope.object.position.addScaledVector(dollyDirection,
//                                                               radiusDelta);
//                         scope.object.updateMatrixWorld();

//                         zoomChanged = !!radiusDelta;

//                     } else if (scope.object.isOrthographicCamera) {

//                         // adjust the ortho camera position based on zoom
//                         // changes
//                         const mouseBefore = new Vector3(mouse.x, mouse.y, 0);
//                         mouseBefore.unproject(scope.object);

//                         const prevZoom    = scope.object.zoom;
//                         scope.object.zoom = Math.max(
//                           scope.minZoom,
//                           Math.min(scope.maxZoom, scope.object.zoom /
//                           scale));
//                         scope.object.updateProjectionMatrix();

//                         zoomChanged = prevZoom != = scope.object.zoom;

//                         const mouseAfter = new Vector3(mouse.x, mouse.y, 0);
//                         mouseAfter.unproject(scope.object);

//                         scope.object.position.sub(mouseAfter).add(mouseBefore);
//                         scope.object.updateMatrixWorld();

//                         newRadius = offset.length();

//                     } else {

//                         console.warn(
//                           'WARNING: OrbitControls.js encountered an unknown
//                           camera type - zoom to cursor disabled.');
//                         scope.zoomToCursor = false;
//                     }

//                     // handle the placement of the target
//                     if (newRadius != = null) {

//                         if (this.screenSpacePanning) {

//                             // position the orbit target in front of the new
//                             // camera position
//                             scope.target.set(0, 0, -1)
//                               .transformDirection(scope.object.matrix)
//                               .multiplyScalar(newRadius)
//                               .add(scope.object.position);

//                         } else {

//                             // get the ray and translation plane to compute
//                             // target
//                             _ray.origin.copy(scope.object.position);
//                             _ray.direction.set(0, 0, -1).transformDirection(
//                               scope.object.matrix);

//                             // if the camera is 20 degrees above the horizon
//                             // then don't adjust the focus target to avoid
//                             // extremely large values
//                             if (Math.abs(scope.object.up.dot(_ray.direction))
//                                 < TILT_LIMIT) {

//                                 object.lookAt(scope.target);

//                             } else {

//                                 _plane.setFromNormalAndCoplanarPoint(
//                                   scope.object.up, scope.target);
//                                 _ray.intersectPlane(_plane, scope.target);
//                             }
//                         }
//                     }

//                 } else if (scope.object.isOrthographicCamera) {

//                     const prevZoom    = scope.object.zoom;
//                     scope.object.zoom = Math.max(
//                       scope.minZoom,
//                       Math.min(scope.maxZoom, scope.object.zoom / scale));

//                     if (prevZoom != = scope.object.zoom) {

//                         scope.object.updateProjectionMatrix();
//                         zoomChanged = true;
//                     }
//                 }

//                 scale             = 1;
//                 performCursorZoom = false;

//                 // update condition is:
//                 // min(camera displacement, camera rotation in radians)^2 >
//                 EPS
//                 // using small-angle approximation cos(x/2) = 1 - x^2 / 8

//                 if (zoomChanged
//                     || lastPosition.distanceToSquared(scope.object.position)
//                          > EPS
//                     || 8 * (1 - lastQuaternion.dot(scope.object.quaternion))
//                          > EPS
//                     || lastTargetPosition.distanceToSquared(scope.target)
//                          > EPS) {

//                     scope.dispatchEvent(_changeEvent);

//                     lastPosition.copy(scope.object.position);
//                     lastQuaternion.copy(scope.object.quaternion);
//                     lastTargetPosition.copy(scope.target);

//                     return true;
//                 }

//                 return false;
//             };
//         }
//         ();

//         //
//         // internals
//         //

//         const scope = this;

//         const STATE = {
//             NONE : -1,
//             ROTATE : 0,
//             DOLLY : 1,
//             PAN : 2,
//             TOUCH_ROTATE : 3,
//             TOUCH_PAN : 4,
//             TOUCH_DOLLY_PAN : 5,
//             TOUCH_DOLLY_ROTATE : 6
//         };

//         let state = STATE.NONE;

//         const EPS = 0.000001;

//         // current position in spherical coordinates
//         const spherical      = new Spherical();
//         const sphericalDelta = new Spherical();

//         let scale       = 1;
//         const panOffset = new Vector3();

//         const rotateStart = new Vector2();
//         const rotateEnd   = new Vector2();
//         const rotateDelta = new Vector2();

//         const panStart = new Vector2();
//         const panEnd   = new Vector2();
//         const panDelta = new Vector2();

//         const dollyStart = new Vector2();
//         const dollyEnd   = new Vector2();
//         const dollyDelta = new Vector2();

//         const dollyDirection  = new Vector3();
//         const mouse           = new Vector2();
//         let performCursorZoom = false;

//         let controlActive = false;

//         function getAutoRotationAngle(deltaTime)
//         {

//             if (deltaTime != = null) {

//                 return (2 * Math.PI / 60 * scope.autoRotateSpeed) *
//                 deltaTime;

//             } else {

//                 return 2 * Math.PI / 60 / 60 * scope.autoRotateSpeed;
//             }
//         }

//         function getZoomScale(delta)
//         {

//             const normalizedDelta = Math.abs(delta * 0.01);
//             return Math.pow(0.95, scope.zoomSpeed * normalizedDelta);
//         }

//         function rotateLeft(angle)
//         {

//             sphericalDelta.theta -= angle;
//         }

//         function rotateUp(angle)
//         {

//             sphericalDelta.phi -= angle;
//         }

//         const panLeft = function()
//         {

//             const v = new Vector3();

//             return function panLeft(distance, objectMatrix)
//             {

//                 v.setFromMatrixColumn(objectMatrix,
//                                       0); // get X column of objectMatrix
//                 v.multiplyScalar(-distance);

//                 panOffset.add(v);
//             };
//         }
//         ();

//         const panUp = function()
//         {

//             const v = new Vector3();

//             return function panUp(distance, objectMatrix)
//             {

//                 if (scope.screenSpacePanning == = true) {

//                     v.setFromMatrixColumn(objectMatrix, 1);

//                 } else {

//                     v.setFromMatrixColumn(objectMatrix, 0);
//                     v.crossVectors(scope.object.up, v);
//                 }

//                 v.multiplyScalar(distance);

//                 panOffset.add(v);
//             };
//         }
//         ();

//         // deltaX and deltaY are in pixels; right and down are positive
//         const pan = function()
//         {

//             const offset = new Vector3();

//             return function pan(deltaX, deltaY)
//             {

//                 const element = scope.domElement;

//                 if (scope.object.isPerspectiveCamera) {

//                     // perspective
//                     const position = scope.object.position;
//                     offset.copy(position).sub(scope.target);
//                     let targetDistance = offset.length();

//                     // half of the fov is center to top of screen
//                     targetDistance
//                       *= Math.tan((scope.object.fov / 2) * Math.PI / 180.0);

//                     // we use only clientHeight here so aspect ratio does not
//                     // distort speed
//                     panLeft(2 * deltaX * targetDistance /
//                     element.clientHeight,
//                             scope.object.matrix);
//                     panUp(2 * deltaY * targetDistance / element.clientHeight,
//                           scope.object.matrix);

//                 } else if (scope.object.isOrthographicCamera) {

//                     // orthographic
//                     panLeft(deltaX * (scope.object.right - scope.object.left)
//                               / scope.object.zoom / element.clientWidth,
//                             scope.object.matrix);
//                     panUp(deltaY * (scope.object.top - scope.object.bottom)
//                             / scope.object.zoom / element.clientHeight,
//                           scope.object.matrix);

//                 } else {

//                     // camera neither orthographic nor perspective
//                     console.warn(
//                       'WARNING: OrbitControls.js encountered an unknown
//                       camera type - pan disabled.');
//                     scope.enablePan = false;
//                 }
//             };
//         }
//         ();

//         function dollyOut(dollyScale)
//         {

//             if (scope.object.isPerspectiveCamera
//                 || scope.object.isOrthographicCamera) {

//                 scale /= dollyScale;

//             } else {

//                 console.warn(
//                   'WARNING: OrbitControls.js encountered an unknown camera
//                   type - dolly/zoom disabled.');
//                 scope.enableZoom = false;
//             }
//         }

//         function dollyIn(dollyScale)
//         {
//             if (scope.object.isPerspectiveCamera
//                 || scope.object.isOrthographicCamera) {
//                 scale *= dollyScale;
//             }
//         }

//         function updateZoomParameters(x, y)
//         {
//             if (!scope.zoomToCursor) {
//                 return;
//             }

//             performCursorZoom = true;

//             const rect = scope.domElement.getBoundingClientRect();
//             const dx   = x - rect.left;
//             const dy   = y - rect.top;
//             const w    = rect.width;
//             const h    = rect.height;

//             mouse.x = (dx / w) * 2 - 1;
//             mouse.y = -(dy / h) * 2 + 1;

//             dollyDirection.set(mouse.x, mouse.y, 1)
//               .unproject(scope.object)
//               .sub(scope.object.position)
//               .normalize();
//         }

//         function clampDistance(dist)
//         {
//             return Math.max(scope.minDistance,
//                             Math.min(scope.maxDistance, dist));
//         }

//         //
//         // event callbacks - update the object state
//         //

//         function handleMouseDownRotate(event)
//         {

//             rotateStart.set(event.clientX, event.clientY);
//         }

//         function handleMouseDownDolly(event)
//         {
//             updateZoomParameters(event.clientX, event.clientX);
//             dollyStart.set(event.clientX, event.clientY);
//         }

//         function handleMouseDownPan(event)
//         {

//             panStart.set(event.clientX, event.clientY);
//         }

//         function handleMouseMoveRotate(event)
//         {

//             rotateEnd.set(event.clientX, event.clientY);

//             rotateDelta.subVectors(rotateEnd, rotateStart)
//               .multiplyScalar(scope.rotateSpeed);

//             const element = scope.domElement;

//             rotateLeft(2 * Math.PI * rotateDelta.x
//                        / element.clientHeight); // yes, height

//             rotateUp(2 * Math.PI * rotateDelta.y / element.clientHeight);

//             rotateStart.copy(rotateEnd);

//             scope.update();
//         }

//         function handleMouseMoveDolly(event)
//         {
//             dollyEnd.set(event.clientX, event.clientY);

//             dollyDelta.subVectors(dollyEnd, dollyStart);

//             if (dollyDelta.y > 0) {

//                 dollyOut(getZoomScale(dollyDelta.y));

//             } else if (dollyDelta.y < 0) {

//                 dollyIn(getZoomScale(dollyDelta.y));
//             }

//             dollyStart.copy(dollyEnd);

//             scope.update();
//         }

//         function handleMouseMovePan(event)
//         {
//             panEnd.set(event.clientX, event.clientY);
//             panDelta.subVectors(panEnd, panStart)
//               .multiplyScalar(scope.panSpeed);
//             pan(panDelta.x, panDelta.y);
//             panStart.copy(panEnd);
//             scope.update();
//         }

//         function handleMouseWheel(event)
//         {
//             updateZoomParameters(event.clientX, event.clientY);
//             if (event.deltaY < 0) {
//                 dollyIn(getZoomScale(event.deltaY));
//             } else if (event.deltaY > 0) {
//                 dollyOut(getZoomScale(event.deltaY));
//             }
//             scope.update();
//         }

//         //
//         // event handlers - FSM: listen for events and reset state
//         //

//         function onMouseDown(event)
//         {

//             let mouseAction;

//             switch (event.button) {
//                 case 0: mouseAction = scope.mouseButtons.LEFT; break;
//                 case 1: mouseAction = scope.mouseButtons.MIDDLE; break;
//                 case 2: mouseAction = scope.mouseButtons.RIGHT; break;
//                 default: mouseAction = -1;
//             }

//             switch (mouseAction) {
//                 case MOUSE.DOLLY:
//                     if (scope.enableZoom == = false) return;
//                     handleMouseDownDolly(event);
//                     state = STATE.DOLLY;
//                     break;
//                 case MOUSE.ROTATE:
//                     if (scope.enableRotate == = false) return;
//                     handleMouseDownRotate(event);
//                     state = STATE.ROTATE;
//                     break;
//                 case MOUSE.PAN:
//                     if (scope.enablePan == = false) return;
//                     handleMouseDownPan(event);
//                     state = STATE.PAN;
//                     break;
//                 default: state = STATE.NONE;
//             }

//             if (state != = STATE.NONE) {
//                 scope.dispatchEvent(_startEvent);
//             }
//         }

//         function onMouseMove(event)
//         {
//             switch (state) {
//                 case STATE.ROTATE:
//                     if (scope.enableRotate == = false) return;
//                     handleMouseMoveRotate(event);
//                     break;
//                 case STATE.DOLLY:
//                     if (scope.enableZoom == = false) return;
//                     handleMouseMoveDolly(event);
//                     break;
//                 case STATE.PAN:
//                     if (scope.enablePan == = false) return;
//                     handleMouseMovePan(event);
//                     break;
//             }
//         }

//         function onMouseWheel(event)
//         {
//             const mode = event.deltaMode;

//             // minimal wheel event altered to meet delta-zoom demand
//             const newEvent = {
//                 clientX : event.clientX,
//                 clientY : event.clientY,
//                 deltaY : event.deltaY,
//             };

//             switch (mode) {
//                 case 1: // LINE_MODE
//                     newEvent.deltaY *= 16;
//                     break;
//                 case 2: // PAGE_MODE
//                     newEvent.deltaY *= 100;
//                     break;
//             }

//             return newEvent;
//         }

//         function onKeyDown(event)
//         {
//             if (scope.enabled == = false || scope.enablePan == = false)
//             return; handleKeyDown(event);
//         }
//     }
// }