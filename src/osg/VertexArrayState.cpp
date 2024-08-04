/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include <osg/VertexArrayState>
#include <osg/State>
#include <osg/ContextData>

using namespace osg;

#if 1
#define VAS_NOTICE OSG_DEBUG
#else
#define VAS_NOTICE OSG_NOTICE
#endif

class VertexArrayStateManager : public GraphicsObjectManager
{
public:
    VertexArrayStateManager(unsigned int contextID):
        GraphicsObjectManager("VertexArrayStateManager", contextID)
    {
    }

    virtual void flushDeletedGLObjects(double, double& availableTime)
    {
        // if no time available don't try to flush objects.
        if (availableTime<=0.0) return;

        VAS_NOTICE<<"VertexArrayStateManager::flushDeletedGLObjects()"<<std::endl;

        const osg::Timer& timer = *osg::Timer::instance();
        osg::Timer_t start_tick = timer.tick();
        double elapsedTime = 0.0;

        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex_vertexArrayStateList);

            // trim from front
            VertexArrayStateList::iterator ditr=_vertexArrayStateList.begin();
            for(;
                ditr!=_vertexArrayStateList.end() && elapsedTime<availableTime;
                ++ditr)
            {
                VertexArrayState* vas = ditr->get();
                vas->deleteVertexArrayObject();

                elapsedTime = timer.delta_s(start_tick,timer.tick());
            }

            if (ditr!=_vertexArrayStateList.begin()) _vertexArrayStateList.erase(_vertexArrayStateList.begin(),ditr);

        }

        elapsedTime = timer.delta_s(start_tick,timer.tick());

        availableTime -= elapsedTime;
    }

    virtual void flushAllDeletedGLObjects()
    {
        VAS_NOTICE<<"VertexArrayStateManager::flushAllDeletedGLObjects()"<<std::endl;

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex_vertexArrayStateList);
        for(VertexArrayStateList::iterator itr = _vertexArrayStateList.begin();
            itr != _vertexArrayStateList.end();
            ++itr)
        {
            VertexArrayState* vas = itr->get();
            vas->deleteVertexArrayObject();
        }
        _vertexArrayStateList.clear();
    }

    virtual void deleteAllGLObjects()
    {
         OSG_INFO<<"VertexArrayStateManager::deleteAllGLObjects() Not currently implemented"<<std::endl;
    }

    virtual void discardAllGLObjects()
    {
        VAS_NOTICE<<"VertexArrayStateManager::flushAllDeletedGLObjects()"<<std::endl;

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex_vertexArrayStateList);
        _vertexArrayStateList.clear();
    }

    void release(VertexArrayState* vas)
    {
        VAS_NOTICE<<"VertexArrayStateManager::release("<<this<<")"<<std::endl;

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex_vertexArrayStateList);
        _vertexArrayStateList.push_back(vas);
    }

protected:

    typedef std::list< osg::ref_ptr<VertexArrayState> > VertexArrayStateList;
    OpenThreads::Mutex _mutex_vertexArrayStateList;
    VertexArrayStateList _vertexArrayStateList;
};

///////////////////////////////////////////////////////////////////////////////////
//
//  VertexAttribArrayDispatch
//

struct VertexAttribArrayDispatch : public VertexArrayState::ArrayDispatch
{
    VertexAttribArrayDispatch(unsigned int in_unit) : unit(in_unit) {}

    virtual const char* className() const { return "VertexAttribArrayDispatch"; }

    virtual bool isVertexAttribDispatch() const { return true; }

    inline void callVertexAttribPointer(GLExtensions* ext, const osg::Array* new_array, const GLvoid * ptr)
    {
        if (new_array->getPreserveDataType())
        {
            if (new_array->getDataType()==GL_FLOAT)
                ext->glVertexAttribPointer(static_cast<GLuint>(unit), new_array->getDataSize(), new_array->getDataType(), new_array->getNormalize(), 0, ptr);
            else if (new_array->getDataType()==GL_DOUBLE)
                ext->glVertexAttribLPointer(static_cast<GLuint>(unit), new_array->getDataSize(), new_array->getDataType(), 0, ptr);
            else
                ext->glVertexAttribIPointer(static_cast<GLuint>(unit), new_array->getDataSize(), new_array->getDataType(), 0, ptr);
        }
        else
        {
            ext->glVertexAttribPointer(static_cast<GLuint>(unit), new_array->getDataSize(), new_array->getDataType(), new_array->getNormalize(), 0, ptr);
        }
    }

    virtual void enable_and_dispatch(osg::State& state, const osg::Array* new_array)
    {
        GLExtensions* ext = state.get<GLExtensions>();

        ext->glEnableVertexAttribArray( unit );
        callVertexAttribPointer(ext, new_array, new_array->getDataPointer());
    }

    virtual void enable_and_dispatch(osg::State& state, const osg::Array* new_array, const osg::GLBufferObject* vbo)
    {
        GLExtensions* ext = state.get<GLExtensions>();

        ext->glEnableVertexAttribArray( unit );
        callVertexAttribPointer(ext, new_array, (const GLvoid *)(vbo->getOffset(new_array->getBufferIndex())));
    }

    virtual void enable_and_dispatch(osg::State& state, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized)
    {
        GLExtensions* ext = state.get<GLExtensions>();

        ext->glEnableVertexAttribArray( unit );
        ext->glVertexAttribPointer(static_cast<GLuint>(unit), size, type, normalized, stride, ptr);
    }

    virtual void dispatch(osg::State& state, const osg::Array* new_array)
    {
        GLExtensions* ext = state.get<GLExtensions>();
        callVertexAttribPointer(ext, new_array, new_array->getDataPointer());
    }

    virtual void dispatch(osg::State& state, const osg::Array* new_array, const osg::GLBufferObject* vbo)
    {
        GLExtensions* ext = state.get<GLExtensions>();
        callVertexAttribPointer(ext, new_array, (const GLvoid *)(vbo->getOffset(new_array->getBufferIndex())));
    }

    virtual void disable(osg::State& state)
    {
        GLExtensions* ext = state.get<GLExtensions>();
        ext->glDisableVertexAttribArray( unit );
    }

    unsigned int unit;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VertexArrayState
//
VertexArrayState::VertexArrayState(osg::State* state):
    _state(state),
    _vertexArrayObject(0),
    _currentVBO(0),
    _currentEBO(0),
    _requiresSetArrays(true)
{
    _stateObserverSet = _state->getOrCreateObserverSet();
    _ext = _state->get<GLExtensions>();
}

VertexArrayState::~VertexArrayState()
{
    if (_stateObserverSet->getObserverdObject())
    {
        _state->resetCurrentVertexArrayStateOnMatch(this);
    }
}

void VertexArrayState::generateVertexArrayObject()
{
    _ext->glGenVertexArrays(1, &_vertexArrayObject);
}

void VertexArrayState::deleteVertexArrayObject()
{
    if (_vertexArrayObject)
    {
        VAS_NOTICE<<"  VertexArrayState::deleteVertexArrayObject() "<<_vertexArrayObject<<" "<<_stateObserverSet->getObserverdObject()<<std::endl;

        _ext->glDeleteVertexArrays(1, &_vertexArrayObject);
        //_vertexArrayObject = 0;
    }
}

bool VertexArrayState::correctArrayDispatchAssigned(const ArrayDispatch* ad)
{
    return ad!=0;
}

namespace {
    VertexArrayState::ArrayDispatch* getOrCreateVertexAttributeDispatch(VertexArrayState::ArrayDispatchList& list, int slot)
    {
        list.resize(slot + 1);
        osg::ref_ptr<VertexArrayState::ArrayDispatch>& ad = list[slot];
        if (!ad.valid())
            ad = new VertexAttribArrayDispatch(slot);

        return ad.get();
    }
}// anonymous namespace

void VertexArrayState::assignVertexArrayDispatcher()
{
    if (correctArrayDispatchAssigned(_vertexArray.get())) return;

    int slot = _state->getVertexAlias()._location;
    VAS_NOTICE << "VertexArrayState::assignVertexArrayDispatcher() _state->getVertexAlias()._location = " << slot << std::endl;
    _vertexArray = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
}

void VertexArrayState::assignNormalArrayDispatcher()
{
    if (correctArrayDispatchAssigned(_normalArray.get())) return;

    int slot = _state->getNormalAlias()._location;
    VAS_NOTICE << "VertexArrayState::assignNormalArrayDispatcher() _state->getNormalAlias()._location = " << slot << std::endl;
    _normalArray = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
}

void VertexArrayState::assignColorArrayDispatcher()
{
    if (correctArrayDispatchAssigned(_colorArray.get())) return;

    int slot = _state->getColorAlias()._location;
    VAS_NOTICE << "VertexArrayState::assignColorArrayDispatcher() _state->getColorAlias()._location = " << slot << std::endl;
    _colorArray = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
}

void VertexArrayState::assignSecondaryColorArrayDispatcher()
{
    if (correctArrayDispatchAssigned(_secondaryColorArray.get())) return;

    int slot = _state->getSecondaryColorAlias()._location;
    VAS_NOTICE << "VertexArrayState::assignSecondaryColorArrayDispatcher() _state->getSecondaryColorAlias()._location = " << slot << std::endl;
    _secondaryColorArray = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
}

void VertexArrayState::assignFogCoordArrayDispatcher()
{
    if (correctArrayDispatchAssigned(_fogCoordArray.get())) return;

    int slot = _state->getFogCoordAlias()._location;
    VAS_NOTICE << "VertexArrayState::assignFogCoordArrayDispatcher() _state->getFogCoordAlias()._location = " << slot << std::endl;
    _fogCoordArray = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
}

void VertexArrayState::assignTexCoordArrayDispatcher(unsigned int numUnits)
{
    _texCoordArrays.resize(numUnits);

    for(unsigned int i=0; i<_texCoordArrays.size(); ++i)
    {
        if (correctArrayDispatchAssigned(_texCoordArrays[i].get())) continue;

        int slot = _state->getTexCoordAliasList()[i]._location;
        VAS_NOTICE << "VertexArrayState::assignTexCoordArrayDispatcher() _state->getTexCoordAliasList()[" << i << "]._location = " << slot << std::endl;
        _texCoordArrays[i] = getOrCreateVertexAttributeDispatch(_vertexAttribArrays, slot);
    }
}

void VertexArrayState::assignVertexAttribArrayDispatcher(unsigned int numUnits)
{
    _vertexAttribArrays.resize(numUnits);

    for(unsigned int i=0; i<_vertexAttribArrays.size(); ++i)
    {
        if (_vertexAttribArrays[i].valid()) continue;

        _vertexAttribArrays[i] = new VertexAttribArrayDispatch(i);
    }
}

void VertexArrayState::assignAllDispatchers()
{
    unsigned int numUnits = 8;
    unsigned int numVertexAttrib = 16;

    assignVertexAttribArrayDispatcher(numVertexAttrib);

    assignVertexArrayDispatcher();
    assignNormalArrayDispatcher();
    assignColorArrayDispatcher();
    assignSecondaryColorArrayDispatcher();
    assignFogCoordArrayDispatcher();
    assignTexCoordArrayDispatcher(numUnits);
}

void VertexArrayState::release()
{
    VAS_NOTICE<<"VertexArrayState::release() "<<this<<std::endl;

    osg::get<VertexArrayStateManager>(_ext->contextID)->release(this);
}

void VertexArrayState::setArray(ArrayDispatch* vad, osg::State& state, const osg::Array* new_array)
{
    if (new_array)
    {
        if (!vad->active)
        {
            vad->active = true;
            _activeDispatchers.push_back(vad);
        }

        if (vad->array==0)
        {
            GLBufferObject* vbo = new_array->getOrCreateGLBufferObject(state.getContextID());
            if (vbo)
            {
                bindVertexBufferObject(vbo);
                vad->enable_and_dispatch(state, new_array, vbo);
            }
            else
            {
                unbindVertexBufferObject();
                vad->enable_and_dispatch(state, new_array);
            }
        }
        else if (new_array!=vad->array || new_array->getModifiedCount()!=vad->modifiedCount)
        {
            GLBufferObject* vbo = new_array->getOrCreateGLBufferObject(state.getContextID());
            if (vbo)
            {
                bindVertexBufferObject(vbo);
                vad->dispatch(state, new_array, vbo);
            }
            else
            {
                unbindVertexBufferObject();
                vad->dispatch(state, new_array);
            }
        }

        vad->array = new_array;
        vad->modifiedCount = new_array->getModifiedCount();

    }
    else if (vad->array)
    {
        disable(vad, state);
    }
}

void VertexArrayState::setArray(ArrayDispatch* vad, osg::State& state, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr, GLboolean normalized)
{
    if(!vad->array){

        if (!vad->active)
        {
            vad->active = true;
            _activeDispatchers.push_back(vad);
        }

        vad->enable_and_dispatch(state, size, type, stride, ptr, normalized);

        vad->modifiedCount = 0xffffffff;

    }
    else
    {
        disable(vad, state);
    }
}

void VertexArrayState::setInterleavedArrays(osg::State& /*state*/, GLenum format, GLsizei stride, const GLvoid* pointer)
{
}

void VertexArrayState::dirty()
{
    setRequiresSetArrays(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
VertexArrayStateList::VertexArrayStateList():
    _array(DisplaySettings::instance()->getMaxNumberOfGraphicsContexts())
{
}


VertexArrayStateList& VertexArrayStateList::operator = (const VertexArrayStateList& rhs)
{
    if (&rhs==this) return *this;

    _array = rhs._array;

    return *this;
}

void VertexArrayStateList::assignAllDispatchers()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignAllDispatchers();
    }
}

void VertexArrayStateList::assignVertexArrayDispatcher()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignVertexArrayDispatcher();
    }
}

void VertexArrayStateList::assignNormalArrayDispatcher()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignNormalArrayDispatcher();
    }
}

void VertexArrayStateList::assignColorArrayDispatcher()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignColorArrayDispatcher();
    }
}

void VertexArrayStateList::assignSecondaryColorArrayDispatcher()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignSecondaryColorArrayDispatcher();
    }
}

void VertexArrayStateList::assignFogCoordArrayDispatcher()
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignFogCoordArrayDispatcher();
    }
}

void VertexArrayStateList::assignTexCoordArrayDispatcher(unsigned int numUnits)
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid())
        {
            (*itr)->assignTexCoordArrayDispatcher(numUnits);
        }
    }
}

void VertexArrayStateList::assignVertexAttribArrayDispatcher(unsigned int numUnits)
{
    for(Array::iterator itr = _array.begin();
        itr != _array.end();
        ++itr)
    {
        if (itr->valid()) (*itr)->assignVertexAttribArrayDispatcher(numUnits);
    }
}
