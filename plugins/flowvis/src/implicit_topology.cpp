#include "stdafx.h"
#include "implicit_topology.h"

#include "implicit_topology_call.h"
#include "implicit_topology_computation.h"
#include "implicit_topology_results.h"
#include "mesh_data_call.h"
#include "triangle_mesh_call.h"
#include "triangulation.h"

#include "mmcore/Call.h"
#include "mmcore/DirectDataWriterCall.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/FilePathParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/LinearTransferFunctionParam.h"

#include "vislib/math/Rectangle.h"
#include "vislib/sys/Log.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

namespace megamol
{
    namespace flowvis
    {
        implicit_topology::implicit_topology() :
            triangle_mesh_slot("set_triangle_mesh", "Triangle mesh output"),
            mesh_data_slot("set_mesh_data", "Mesh data output"),
            result_writer_slot("result_writer_slot", "Results output slot"),
            log_slot("log_slot", "Log output slot"),
            performance_slot("performance_slot", "Performance log output slot"),
            result_reader_slot("result_reader_slot", "Results input slot"),
            start_computation("start_computation", "Start the computation"),
            stop_computation("stop_computation", "Stop the computation"),
            reset_computation("reset_computation", "Reset the computation"),
            load_computation("load_computation", "Load computation from file"),
            save_computation("save_computation", "Save computation to file"),
            vector_field_path("vector_field_path", "Path to the input vector field"),
            convergence_structures_path("convergence_structures_path", "Path to the input convergence structures"),
            label_transfer_function("label_transfer_function", "Transfer function for labels"),
            label_fixed_range("label_fixed_range", "Fixed or dynamic value range for labels"),
            label_range_min("label_range_min", "Minimum value for labels in the transfer function"),
            label_range_max("label_range_max", "Maximum value for labels in the transfer function"),
            distance_transfer_function("distance_transfer_function", "Transfer function for distances"),
            distance_fixed_range("distance_fixed_range", "Fixed or dynamic value range for labels"),
            distance_range_min("distance_range_min", "Minimum value for distances in the transfer function"),
            distance_range_max("distance_range_max", "Maximum value for distances in the transfer function"),
            termination_transfer_function("termination_transfer_function", "Transfer function for reasons of termination"),
            termination_fixed_range("termination_fixed_range", "Fixed or dynamic value range for reasons of termination"),
            termination_range_min("termination_range_min", "Minimum value for reasons of termination in the transfer function"),
            termination_range_max("termination_range_max", "Maximum value for reasons of termination in the transfer function"),
            gradient_transfer_function("gradient_transfer_function", "Transfer function for gradients"),
            gradient_fixed_range("gradient_fixed_range", "Fixed or dynamic value range for gradients"),
            gradient_range_min("gradient_range_min", "Minimum value for gradients in the transfer function"),
            gradient_range_max("gradient_range_max", "Maximum value for gradients in the transfer function"),
            num_integration_steps("num_integration_steps", "Number of stream line integration steps"),
            integration_timestep("integration_timestep", "Initial time step for stream line integration"),
            max_integration_error("max_integration_error", "Maximum integration error for Runge-Kutta 4-5"),
            num_particles_per_batch("num_particles_per_batch", "Number of particles per batch (influences GPU utilization)"),
            num_integration_steps_per_batch("num_integration_steps_per_batch", "Number of integration steps per batch, after which a result can be visualized"),
            refinement_threshold("refinement_threshold", "Threshold for grid refinement, defined as minimum edge length"),
            refine_at_labels("refine_at_labels", "Should the grid be refined in regions of different labels?"),
            distance_difference_threshold("distance_difference_threshold", "Threshold for refining the grid when neighboring nodes exceed a distance difference"),
            computation_running(false), mesh_output_changed(false), data_output_changed(false), computation(nullptr), previous_result(nullptr)
        {
            // Connect output
            this->triangle_mesh_slot.SetCallback(triangle_mesh_call::ClassName(), triangle_mesh_call::FunctionName(0), &implicit_topology::get_triangle_data_callback);
            this->triangle_mesh_slot.SetCallback(triangle_mesh_call::ClassName(), triangle_mesh_call::FunctionName(1), &implicit_topology::get_triangle_extent_callback);
            this->MakeSlotAvailable(&this->triangle_mesh_slot);

            this->mesh_data_slot.SetCallback(mesh_data_call::ClassName(), mesh_data_call::FunctionName(0), &implicit_topology::get_data_data_callback);
            this->mesh_data_slot.SetCallback(mesh_data_call::ClassName(), mesh_data_call::FunctionName(1), &implicit_topology::get_data_extent_callback);
            this->MakeSlotAvailable(&this->mesh_data_slot);

            this->result_writer_slot.SetCallback(implicit_topology_writer_call::ClassName(), implicit_topology_writer_call::FunctionName(0), &implicit_topology::get_result_writer_cb_callback);
            this->MakeSlotAvailable(&this->result_writer_slot);
            this->get_result_writer_callback = [](const implicit_topology_results&) -> bool
                { vislib::sys::Log::DefaultLog.WriteWarn("Cannot write results. Writer module not connected!"); return true; };
            
            this->log_slot.SetCallback(core::DirectDataWriterCall::ClassName(), core::DirectDataWriterCall::FunctionName(0), &implicit_topology::get_log_cb_callback);
            this->MakeSlotAvailable(&this->log_slot);
            this->get_log_callback = []() -> std::ostream& { static std::ostream dummy(nullptr); return dummy; };

            this->performance_slot.SetCallback(core::DirectDataWriterCall::ClassName(), core::DirectDataWriterCall::FunctionName(0), &implicit_topology::get_performance_cb_callback);
            this->MakeSlotAvailable(&this->performance_slot);
            this->get_performance_callback = []() -> std::ostream& { static std::ostream dummy(nullptr); return dummy; };

            // Connect input
            this->result_reader_slot.SetCompatibleCall<implicit_topology_reader_call::implicit_topology_reader_description>();
            this->MakeSlotAvailable(&this->result_reader_slot);

            // Create path parameters
            this->vector_field_path << new core::param::FilePathParam("");
            this->MakeSlotAvailable(&this->vector_field_path);

            this->convergence_structures_path << new core::param::FilePathParam("");
            this->MakeSlotAvailable(&this->convergence_structures_path);

            // Create computation parameters
            this->num_integration_steps << new core::param::IntParam(0);
            this->MakeSlotAvailable(&this->num_integration_steps);

            this->integration_timestep << new core::param::FloatParam(0.01f);
            this->MakeSlotAvailable(&this->integration_timestep);

            this->max_integration_error << new core::param::FloatParam(0.000001f);
            this->MakeSlotAvailable(&this->max_integration_error);

            this->num_particles_per_batch << new core::param::IntParam(10000);
            this->MakeSlotAvailable(&this->num_particles_per_batch);

            this->num_integration_steps_per_batch << new core::param::IntParam(10000);
            this->MakeSlotAvailable(&this->num_integration_steps_per_batch);

            this->refinement_threshold << new core::param::FloatParam(0.00024f);
            this->MakeSlotAvailable(&this->refinement_threshold);

            this->refine_at_labels << new core::param::BoolParam(true);
            this->MakeSlotAvailable(&this->refine_at_labels);

            this->distance_difference_threshold << new core::param::FloatParam(0.00025f);
            this->MakeSlotAvailable(&this->distance_difference_threshold);

            // Create computation buttons
            this->start_computation << new core::param::ButtonParam();
            this->start_computation.SetUpdateCallback(&implicit_topology::start_computation_callback);
            this->MakeSlotAvailable(&this->start_computation);

            this->stop_computation << new core::param::ButtonParam();
            this->stop_computation.SetUpdateCallback(&implicit_topology::stop_computation_callback);
            this->MakeSlotAvailable(&this->stop_computation);

            this->reset_computation << new core::param::ButtonParam();
            this->reset_computation.SetUpdateCallback(&implicit_topology::reset_computation_callback);
            this->MakeSlotAvailable(&this->reset_computation);

            this->load_computation << new core::param::ButtonParam();
            this->load_computation.SetUpdateCallback(&implicit_topology::load_computation_callback);
            this->MakeSlotAvailable(&this->load_computation);

            this->save_computation << new core::param::ButtonParam();
            this->save_computation.SetUpdateCallback(&implicit_topology::save_computation_callback);
            this->MakeSlotAvailable(&this->save_computation);

            // Create transfer function parameters
            this->label_transfer_function << new core::param::LinearTransferFunctionParam(
                "{\"Interpolation\":\"LINEAR\",\"Nodes\":[[0.0,0.0,0.423499,1.0,0.0],[0.0,0.119346,0.529237,1.0,0.125]," \
                "[0.0,0.238691,0.634976,1.0,0.1875],[0.0,0.346852,0.68788,1.0,0.25],[0.0,0.45022,0.718141,1.0,0.3125]," \
                "[0.0,0.553554,0.664839,1.0,0.375],[0.0,0.651082,0.519303,1.0,0.4375],[0.115841,0.72479,0.352857,1.0,0.5]," \
                "[0.326771,0.781195,0.140187,1.0,0.5625],[0.522765,0.798524,0.0284624,1.0,0.625],[0.703162,0.788685,0.00885756,1.0,0.6875]," \
                "[0.845118,0.751133,0.0,1.0,0.75],[0.955734,0.690825,0.0,1.0,0.8125],[0.995402,0.567916,0.0618524,1.0,0.875]," \
                "[0.987712,0.403398,0.164851,1.0,0.9375],[0.980407,0.247105,0.262699,1.0,1.0]],\"TextureSize\":128}");
            this->MakeSlotAvailable(&this->label_transfer_function);

            this->label_fixed_range << new core::param::BoolParam(false);
            this->MakeSlotAvailable(&this->label_fixed_range);

            this->label_range_min << new core::param::FloatParam(0.0f);
            this->MakeSlotAvailable(&this->label_range_min);

            this->label_range_max << new core::param::FloatParam(1.0f);
            this->MakeSlotAvailable(&this->label_range_max);

            this->distance_transfer_function << new core::param::LinearTransferFunctionParam(
                "{\"Interpolation\":\"LINEAR\",\"Nodes\":[[0.0,0.0,0.0,1.0,0.0],[0.9019607901573181,0.0,0.0,1.0,0.39500004053115845]," \
                "[0.9019607901573181,0.9019607901573181,0.0,1.0,0.7990000247955322],[1.0,1.0,1.0,1.0,1.0]],\"TextureSize\":128}");
            this->MakeSlotAvailable(&this->distance_transfer_function);

            this->distance_fixed_range << new core::param::BoolParam(false);
            this->MakeSlotAvailable(&this->distance_fixed_range);

            this->distance_range_min << new core::param::FloatParam(0.0f);
            this->MakeSlotAvailable(&this->distance_range_min);

            this->distance_range_max << new core::param::FloatParam(1.0f);
            this->MakeSlotAvailable(&this->distance_range_max);

            this->termination_transfer_function << new core::param::LinearTransferFunctionParam(
                "{\"Interpolation\":\"LINEAR\",\"Nodes\":[[0.23137255012989044,0.2980392277240753,0.7529411911964417,1.0,0.0]," \
                "[0.8627451062202454,0.8627451062202454,0.8627451062202454,1.0,0.4989999830722809]," \
                "[0.7058823704719543,0.01568627543747425,0.14901961386203766,1.0,1.0]],\"TextureSize\":4}");
            this->MakeSlotAvailable(&this->termination_transfer_function);

            this->termination_fixed_range << new core::param::BoolParam(true);
            this->MakeSlotAvailable(&this->termination_fixed_range);

            this->termination_range_min << new core::param::FloatParam(-1.0f);
            this->MakeSlotAvailable(&this->termination_range_min);

            this->termination_range_max << new core::param::FloatParam(2.0f);
            this->MakeSlotAvailable(&this->termination_range_max);

            this->gradient_transfer_function << new core::param::LinearTransferFunctionParam(
                "{\"Interpolation\":\"LINEAR\",\"Nodes\":[[1.0,1.0,1.0,1.0,0.0],[0.0,0.0,0.0,1.0,1.0]],\"TextureSize\":128}");
            this->MakeSlotAvailable(&this->gradient_transfer_function);

            this->gradient_fixed_range << new core::param::BoolParam(false);
            this->MakeSlotAvailable(&this->gradient_fixed_range);

            this->gradient_range_min << new core::param::FloatParam(0.0f);
            this->MakeSlotAvailable(&this->gradient_range_min);

            this->gradient_range_max << new core::param::FloatParam(1.0f);
            this->MakeSlotAvailable(&this->gradient_range_max);
        }

        implicit_topology::~implicit_topology()
        {
            this->Release();

            if (this->computation != nullptr)
            {
                this->computation->terminate();
            }
        }

        bool implicit_topology::create()
        {
            return true;
        }

        void implicit_topology::release()
        {
        }

        bool implicit_topology::initialize_computation()
        {
            // Try to load input vector field
            if (this->computation == nullptr)
            {
                std::array<int, 2> resolution;
                std::array<float, 4> domain;
                
                std::vector<float> positions;
                std::vector<float> vectors;
                std::vector<float> points;
                std::vector<int> point_ids;
                std::vector<float> lines;
                std::vector<int> line_ids;

                if (load_input(resolution, domain, positions, vectors, points, point_ids, lines, line_ids))
                {
                    // Create new computation object
                    this->computation = std::make_unique<implicit_topology_computation>(this->get_log_callback(), this->get_performance_callback(),
                        std::move(resolution), std::move(domain), std::move(positions), std::move(vectors), std::move(points), std::move(point_ids),
                        std::move(lines), std::move(line_ids), this->integration_timestep.Param<core::param::FloatParam>()->Value(),
                        this->max_integration_error.Param<core::param::FloatParam>()->Value());

                    set_readonly_fixed_parameters(true);

                    return true;
                }

                return false;
            }

            return true;
        }

        bool implicit_topology::load_input(std::array<int, 2>& resolution, std::array<float, 4>& domain, std::vector<float>& positions,
            std::vector<float>& vectors, std::vector<float>& points, std::vector<int>& point_ids, std::vector<float>& lines, std::vector<int>& line_ids)
        {
            std::ifstream vectors_ifs(this->vector_field_path.Param<core::param::FilePathParam>()->Value(), std::ios_base::in | std::ios_base::binary);
            std::ifstream structures_ifs(this->convergence_structures_path.Param<core::param::FilePathParam>()->Value(), std::ios_base::in | std::ios_base::binary);

            if (vectors_ifs.good() && structures_ifs.good())
            {
                // Get dimension from file
                unsigned int dimension, components;

                vectors_ifs.read(reinterpret_cast<char*>(&dimension), sizeof(unsigned int));
                vectors_ifs.read(reinterpret_cast<char*>(&components), sizeof(unsigned int));

                if (dimension != 2)
                {
                    vislib::sys::Log::DefaultLog.WriteError("Vector field file must have exactly two dimensions '%s'",
                        this->vector_field_path.Param<core::param::FilePathParam>()->Value());

                    return false;
                }

                if (components != 2)
                {
                    vislib::sys::Log::DefaultLog.WriteError("Vectors must have exactly two components '%s'",
                        this->vector_field_path.Param<core::param::FilePathParam>()->Value());

                    return false;
                }

                // Read extents from file
                float x_min, x_max, y_min, y_max;
                unsigned int x_num, y_num, num;

                vectors_ifs.read(reinterpret_cast<char*>(&x_num), sizeof(unsigned int));
                vectors_ifs.read(reinterpret_cast<char*>(&x_min), sizeof(float));
                vectors_ifs.read(reinterpret_cast<char*>(&x_max), sizeof(float));
                vectors_ifs.read(reinterpret_cast<char*>(&y_num), sizeof(unsigned int));
                vectors_ifs.read(reinterpret_cast<char*>(&y_min), sizeof(float));
                vectors_ifs.read(reinterpret_cast<char*>(&y_max), sizeof(float));

                num = x_num * y_num;

                this->resolution = resolution = { static_cast<int>(x_num), static_cast<int>(y_num) };
                domain = { x_min, x_max, y_min, y_max };

                // Read file content
                const float x_step = (x_max - x_min) / (x_num - 1);
                const float y_step = (y_max - y_min) / (y_num - 1);

                positions.resize(num * 2);
                vectors.resize(num * 2);

                for (unsigned int y = 0; y < y_num; ++y)
                {
                    for (unsigned int x = 0; x < x_num; ++x)
                    {
                        const unsigned int xy = y * x_num + x;

                        // Calculate positions
                        const float x_pos = x_min + x * x_step;
                        const float y_pos = y_min + y * y_step;

                        positions[xy * 2 + 0] = x_pos;
                        positions[xy * 2 + 1] = y_pos;

                        // Read vectors
                        vectors_ifs.read(reinterpret_cast<char*>(&vectors[xy * 2 + 0]), sizeof(float));
                        vectors_ifs.read(reinterpret_cast<char*>(&vectors[xy * 2 + 1]), sizeof(float));
                    }
                }

                vectors_ifs.close();

                // Load convergence structures
                unsigned int num_convergence_structures;

                structures_ifs.read(reinterpret_cast<char*>(&num_convergence_structures), sizeof(unsigned int));

                points.reserve(2 * num_convergence_structures);
                lines.reserve(4 * num_convergence_structures);

                point_ids.reserve(num_convergence_structures);
                line_ids.reserve(num_convergence_structures);

                for (unsigned int i = 0; i < num_convergence_structures; ++i)
                {
                    unsigned int type;
                    structures_ifs.read(reinterpret_cast<char*>(&type), sizeof(unsigned int));

                    float value;

                    switch (type)
                    {
                    case 0: // Point
                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        points.push_back(value);
                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        points.push_back(value);

                        point_ids.push_back(i);

                        break;
                    case 1: // Line
                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        lines.push_back(value);
                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        lines.push_back(value);

                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        lines.push_back(value);
                        structures_ifs.read(reinterpret_cast<char*>(&value), sizeof(float));
                        lines.push_back(value);

                        line_ids.push_back(i);

                        break;
                    default:
                        vislib::sys::Log::DefaultLog.WriteError("Unknown convergence structure type in file '%s'!",
                            this->convergence_structures_path.Param<core::param::FilePathParam>()->Value());

                        return false;
                    }
                }
            }
            else if (!vectors_ifs.good())
            {
                vislib::sys::Log::DefaultLog.WriteWarn("Unable to open input vector field file '%s'!",
                    this->vector_field_path.Param<core::param::FilePathParam>()->Value());

                return false;
            }
            else if (!structures_ifs.good())
            {
                vislib::sys::Log::DefaultLog.WriteWarn("Unable to open input convergence structures file '%s'!",
                    this->convergence_structures_path.Param<core::param::FilePathParam>()->Value());

                return false;
            }

            return true;
        }

        void implicit_topology::update_results()
        {
            // Try to get new results
            if (this->computation_running && !(this->mesh_output_changed || this->data_output_changed))
            {
                // Get new results
                if (this->last_result.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                {
                    return;
                }

                vislib::sys::Log::DefaultLog.WriteInfo("Computation of topology yielded new results.");

                // Store triangles
                auto result = this->last_result.get();

                this->vertices = result.vertices;
                this->indices = result.indices;

                this->labels_forward = result.labels_forward;
                this->distances_forward = result.distances_forward;
                this->terminations_forward = result.terminations_forward;

                this->labels_backward = result.labels_backward;
                this->distances_backward = result.distances_backward;
                this->terminations_backward = result.terminations_backward;

                this->computation_running = !result.computation_state.finished;

                if (result.computation_state.finished)
                {
                    vislib::sys::Log::DefaultLog.WriteInfo("Computation of topology ended.");

                    // Reset parameters to read-write
                    set_readonly_variable_parameters(false);
                }

                // Save new last result
                this->last_result = this->computation->get_results();
                this->previous_result = std::make_unique<implicit_topology_results>(result);

                this->mesh_output_changed = true;
                this->data_output_changed = true;
            }
        }

        void implicit_topology::set_readonly_fixed_parameters(const bool read_only)
        {
            this->vector_field_path.Parameter()->SetGUIReadOnly(read_only);
            this->convergence_structures_path.Parameter()->SetGUIReadOnly(read_only);

            this->integration_timestep.Parameter()->SetGUIReadOnly(read_only);
            this->max_integration_error.Parameter()->SetGUIReadOnly(read_only);
        }

        void implicit_topology::set_readonly_variable_parameters(const bool read_only)
        {
            this->num_integration_steps.Parameter()->SetGUIReadOnly(read_only);
            this->num_particles_per_batch.Parameter()->SetGUIReadOnly(read_only);
            this->num_integration_steps_per_batch.Parameter()->SetGUIReadOnly(read_only);

            this->refinement_threshold.Parameter()->SetGUIReadOnly(read_only);
            this->refine_at_labels.Parameter()->SetGUIReadOnly(read_only);
            this->distance_difference_threshold.Parameter()->SetGUIReadOnly(read_only);
        }

        bool implicit_topology::get_triangle_data_callback(core::Call& call)
        {
            auto* triangle_call = dynamic_cast<triangle_mesh_call*>(&call);
            if (triangle_call == nullptr) return false;

            // Update render output if there are new results
            update_results();

            if (this->mesh_output_changed)
            {
                triangle_call->set_vertices(this->vertices);
                triangle_call->set_indices(this->indices);

                triangle_call->SetDataHash(triangle_call->DataHash() + 1);

                this->mesh_output_changed = false;
            }
            
            return true;
        }

        bool implicit_topology::get_triangle_extent_callback(core::Call& call)
        {
            auto* triangle_call = dynamic_cast<triangle_mesh_call*>(&call);
            if (triangle_call == nullptr) return false;

            if (this->vector_field_path.IsDirty())
            {
                // Try to load input vector field
                std::ifstream vectors_ifs(this->vector_field_path.Param<core::param::FilePathParam>()->Value(), std::ios_base::in | std::ios_base::binary);

                if (vectors_ifs.good())
                {
                    // Read dimension from file
                    unsigned int dimension, components;

                    vectors_ifs.read(reinterpret_cast<char*>(&dimension), sizeof(unsigned int));
                    vectors_ifs.read(reinterpret_cast<char*>(&components), sizeof(unsigned int));

                    if (dimension != 2)
                    {
                        vislib::sys::Log::DefaultLog.WriteError("Vector field file must have exactly two dimensions '%s'",
                            this->vector_field_path.Param<core::param::FilePathParam>()->Value());

                        return false;
                    }

                    if (components != 2)
                    {
                        vislib::sys::Log::DefaultLog.WriteError("Vectors must have exactly two components '%s'",
                            this->vector_field_path.Param<core::param::FilePathParam>()->Value());

                        return false;
                    }

                    // Read extents from file
                    float x_min, x_max, y_min, y_max;

                    vectors_ifs.ignore(sizeof(unsigned int));
                    vectors_ifs.read(reinterpret_cast<char*>(&x_min), sizeof(float));
                    vectors_ifs.read(reinterpret_cast<char*>(&x_max), sizeof(float));
                    vectors_ifs.ignore(sizeof(unsigned int));
                    vectors_ifs.read(reinterpret_cast<char*>(&y_min), sizeof(float));
                    vectors_ifs.read(reinterpret_cast<char*>(&y_max), sizeof(float));

                    triangle_call->set_bounding_rectangle(vislib::math::Rectangle<float>(x_min, y_min, x_max, y_max));
                }
                else
                {
                    triangle_call->SetDataHash(0);

                    this->vector_field_path.ResetDirty();

                    return false;
                }
            }

            return true;
        }

        bool implicit_topology::get_data_data_callback(core::Call& call)
        {
            auto* data_call = dynamic_cast<mesh_data_call*>(&call);
            if (data_call == nullptr) return false;

            // Only update if there is actual data
            if (this->labels_forward == nullptr || this->labels_backward == nullptr || this->distances_forward == nullptr ||
                this->distances_backward == nullptr || this->terminations_forward == nullptr || this->terminations_backward == nullptr)
            {
                return true;
            }

            // Update render output if there are new results
            update_results();

            if (this->data_output_changed
                || this->label_fixed_range.IsDirty() || this->label_range_min.IsDirty() || this->label_range_max.IsDirty()
                || this->distance_fixed_range.IsDirty() || this->distance_range_min.IsDirty() || this->distance_range_max.IsDirty()
                || this->termination_fixed_range.IsDirty() || this->termination_range_min.IsDirty() || this->termination_range_max.IsDirty()
                || this->gradient_fixed_range.IsDirty() || this->gradient_range_min.IsDirty() || this->gradient_range_max.IsDirty())
            {
                this->label_fixed_range.ResetDirty();
                this->label_range_min.ResetDirty();
                this->label_range_max.ResetDirty();

                this->distance_fixed_range.ResetDirty();
                this->distance_range_min.ResetDirty();
                this->distance_range_max.ResetDirty();

                this->termination_fixed_range.ResetDirty();
                this->termination_range_min.ResetDirty();
                this->termination_range_max.ResetDirty();

                this->gradient_fixed_range.ResetDirty();
                this->gradient_range_min.ResetDirty();
                this->gradient_range_max.ResetDirty();

                // Set data function
                auto set_data = [](mesh_data_call* call, std::shared_ptr<std::vector<float>> data, const std::string& name,
                    const bool fixed_range, const float range_min, const float range_max) -> std::pair<float, float>
                {
                    auto data_set = std::make_shared<mesh_data_call::data_set>();

                    if (fixed_range)
                    {
                        data_set->min_value = range_min;
                        data_set->max_value = range_max;
                    }
                    else
                    {
                        const auto min_max_value = std::minmax_element(data->begin(), data->end());
                        data_set->min_value = *min_max_value.first;
                        data_set->max_value = *min_max_value.second;
                    }

                    data_set->data = data;

                    call->set_data(name, data_set);

                    return { data_set->min_value, data_set->max_value };
                };

                // Set labels
                auto label_forward_min_max = set_data(data_call, this->labels_forward, "labels (forward)",
                    this->label_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->label_range_min.Param<core::param::FloatParam>()->Value(),
                    this->label_range_max.Param<core::param::FloatParam>()->Value());

                auto label_backward_min_max = set_data(data_call, this->labels_backward, "labels (backward)",
                    this->label_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->label_range_min.Param<core::param::FloatParam>()->Value(),
                    this->label_range_max.Param<core::param::FloatParam>()->Value());

                const float label_min = std::min(label_forward_min_max.first, label_backward_min_max.first);
                const float label_max = std::max(label_forward_min_max.second, label_backward_min_max.second);

                this->label_range_min.Param<core::param::FloatParam>()->SetValue(label_min, false);
                this->label_range_max.Param<core::param::FloatParam>()->SetValue(label_max, false);

                this->label_transfer_function.ForceSetDirty();
                
                // Set distances
                auto distance_forward_min_max = set_data(data_call, this->distances_forward, "distances (forward)",
                    this->distance_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->distance_range_min.Param<core::param::FloatParam>()->Value(),
                    this->distance_range_max.Param<core::param::FloatParam>()->Value());

                auto distance_backward_min_max = set_data(data_call, this->distances_backward, "distances (backward)",
                    this->distance_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->distance_range_min.Param<core::param::FloatParam>()->Value(),
                    this->distance_range_max.Param<core::param::FloatParam>()->Value());

                const float distance_min = std::min(distance_forward_min_max.first, distance_backward_min_max.first);
                const float distance_max = std::max(distance_forward_min_max.second, distance_backward_min_max.second);

                this->distance_range_min.Param<core::param::FloatParam>()->SetValue(distance_min, false);
                this->distance_range_max.Param<core::param::FloatParam>()->SetValue(distance_max, false);

                this->distance_transfer_function.ForceSetDirty();

                // Set reasons for termination
                auto termination_forward_min_max = set_data(data_call, this->terminations_forward, "terminations (forward)",
                    this->termination_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->termination_range_min.Param<core::param::FloatParam>()->Value(),
                    this->termination_range_max.Param<core::param::FloatParam>()->Value());

                auto termination_backward_min_max = set_data(data_call, this->terminations_backward, "terminations (backward)",
                    this->termination_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->termination_range_min.Param<core::param::FloatParam>()->Value(),
                    this->termination_range_max.Param<core::param::FloatParam>()->Value());

                const float termination_min = std::min(termination_forward_min_max.first, termination_backward_min_max.first);
                const float termination_max = std::max(termination_forward_min_max.second, termination_backward_min_max.second);

                this->termination_range_min.Param<core::param::FloatParam>()->SetValue(termination_min, false);
                this->termination_range_max.Param<core::param::FloatParam>()->SetValue(termination_max, false);

                this->termination_transfer_function.ForceSetDirty();

                // Compute and set gradient magnitudes
                if (this->data_output_changed)
                {
                    this->gradients_forward = std::make_shared<std::vector<float>>(this->distances_forward->size());
                    this->gradients_backward = std::make_shared<std::vector<float>>(this->distances_backward->size());

                    const std::vector<float>& distances_forward = *this->distances_forward;
                    const std::vector<float>& distances_backward = *this->distances_backward;

                    for (int x = 0; x < this->resolution[0]; ++x)
                    {
                        for (int y = 0; y < this->resolution[1]; ++y)
                        {
                            const int index = x + y * resolution[0];

                            float magnitude_forward = 0.0f;
                            float magnitude_backward = 0.0f;

                            if (x == 0)
                            {
                                magnitude_forward += std::pow(distances_forward[index + 1] - distances_forward[index], 2.0f);
                                magnitude_backward += std::pow(distances_backward[index + 1] - distances_backward[index], 2.0f);
                            }
                            else if (x == this->resolution[0] - 1)
                            {
                                magnitude_forward += std::pow(distances_forward[index] - distances_forward[index - 1], 2.0f);
                                magnitude_backward += std::pow(distances_backward[index] - distances_backward[index - 1], 2.0f);
                            }
                            else
                            {
                                magnitude_forward += std::pow((distances_forward[index + 1] - distances_forward[index - 1]) / 2.0f, 2.0f);
                                magnitude_backward += std::pow((distances_backward[index + 1] - distances_backward[index - 1]) / 2.0f, 2.0f);
                            }

                            if (y == 0)
                            {
                                magnitude_forward += std::pow(distances_forward[index + this->resolution[0]] - distances_forward[index], 2.0f);
                                magnitude_backward += std::pow(distances_backward[index + this->resolution[0]] - distances_backward[index], 2.0f);
                            }
                            else if (y == this->resolution[1] - 1)
                            {
                                magnitude_forward += std::pow(distances_forward[index] - distances_forward[index - this->resolution[0]], 2.0f);
                                magnitude_backward += std::pow(distances_backward[index] - distances_backward[index - this->resolution[0]], 2.0f);
                            }
                            else
                            {
                                magnitude_forward += std::pow((distances_forward[index + this->resolution[0]] - distances_forward[index - this->resolution[0]]) / 2.0f, 2.0f);
                                magnitude_backward += std::pow((distances_backward[index + this->resolution[0]] - distances_backward[index - this->resolution[0]]) / 2.0f, 2.0f);
                            }

                            (*this->gradients_forward)[index] = std::sqrt(magnitude_forward);
                            (*this->gradients_backward)[index] = std::sqrt(magnitude_backward);
                        }
                    }
                }

                auto gradient_forward_min_max = set_data(data_call, this->gradients_forward, "gradients (forward)",
                    this->gradient_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->gradient_range_min.Param<core::param::FloatParam>()->Value(),
                    this->gradient_range_max.Param<core::param::FloatParam>()->Value());

                auto gradient_backward_min_max = set_data(data_call, this->gradients_backward, "gradients (backward)",
                    this->gradient_fixed_range.Param<core::param::BoolParam>()->Value(),
                    this->gradient_range_min.Param<core::param::FloatParam>()->Value(),
                    this->gradient_range_max.Param<core::param::FloatParam>()->Value());

                const float gradient_min = std::min(gradient_forward_min_max.first, gradient_backward_min_max.first);
                const float gradient_max = std::max(gradient_forward_min_max.second, gradient_backward_min_max.second);

                this->gradient_range_min.Param<core::param::FloatParam>()->SetValue(gradient_min, false);
                this->gradient_range_max.Param<core::param::FloatParam>()->SetValue(gradient_max, false);

                this->gradient_transfer_function.ForceSetDirty();

                // Set new data hash
                data_call->SetDataHash(data_call->DataHash() + 1);

                this->data_output_changed = false;
            }

            // Set accessibility
            this->label_range_min.Parameter()->SetGUIReadOnly(!this->label_fixed_range.Param<core::param::BoolParam>()->Value());
            this->label_range_max.Parameter()->SetGUIReadOnly(!this->label_fixed_range.Param<core::param::BoolParam>()->Value());

            this->distance_range_min.Parameter()->SetGUIReadOnly(!this->distance_fixed_range.Param<core::param::BoolParam>()->Value());
            this->distance_range_max.Parameter()->SetGUIReadOnly(!this->distance_fixed_range.Param<core::param::BoolParam>()->Value());

            this->termination_range_min.Parameter()->SetGUIReadOnly(!this->termination_fixed_range.Param<core::param::BoolParam>()->Value());
            this->termination_range_max.Parameter()->SetGUIReadOnly(!this->termination_fixed_range.Param<core::param::BoolParam>()->Value());

            this->gradient_range_min.Parameter()->SetGUIReadOnly(!this->gradient_fixed_range.Param<core::param::BoolParam>()->Value());
            this->gradient_range_max.Parameter()->SetGUIReadOnly(!this->gradient_fixed_range.Param<core::param::BoolParam>()->Value());

            // Update transfer functions
            auto set_transfer_function = [](std::shared_ptr<mesh_data_call::data_set> data_set, std::string transfer_function)
            {
                if (data_set != nullptr)
                {
                    std::swap(data_set->transfer_function, transfer_function);

                    data_set->transfer_function_dirty = true;
                }
            };

            if (this->label_transfer_function.IsDirty())
            {
                set_transfer_function(data_call->get_data("labels (forward)"), this->label_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());
                set_transfer_function(data_call->get_data("labels (backward)"), this->label_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());

                this->label_transfer_function.ResetDirty();
            }

            if (this->distance_transfer_function.IsDirty())
            {
                set_transfer_function(data_call->get_data("distances (forward)"), this->distance_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());
                set_transfer_function(data_call->get_data("distances (backward)"), this->distance_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());

                this->distance_transfer_function.ResetDirty();
            }

            if (this->termination_transfer_function.IsDirty())
            {
                set_transfer_function(data_call->get_data("terminations (forward)"), this->termination_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());
                set_transfer_function(data_call->get_data("terminations (backward)"), this->termination_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());

                this->termination_transfer_function.ResetDirty();
            }

            if (this->gradient_transfer_function.IsDirty())
            {
                set_transfer_function(data_call->get_data("gradients (forward)"), this->gradient_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());
                set_transfer_function(data_call->get_data("gradients (backward)"), this->gradient_transfer_function.Param<core::param::LinearTransferFunctionParam>()->Value());

                this->gradient_transfer_function.ResetDirty();
            }

            return true;
        }

        bool implicit_topology::get_data_extent_callback(core::Call& call)
        {
            auto* data_call = dynamic_cast<mesh_data_call*>(&call);
            if (data_call == nullptr) return false;

            data_call->set_data("labels (forward)");
            data_call->set_data("labels (backward)");

            data_call->set_data("distances (forward)");
            data_call->set_data("distances (backward)");

            data_call->set_data("reasons for termination (forward)");
            data_call->set_data("reasons for termination (backward)");

            data_call->set_data("gradients (forward)");
            data_call->set_data("gradients (backward)");

            return true;
        }

        bool implicit_topology::get_result_writer_cb_callback(core::Call& call)
        {
            this->get_result_writer_callback = dynamic_cast<implicit_topology_writer_call*>(&call)->GetCallback();

            return true;
        }

        bool implicit_topology::get_log_cb_callback(core::Call& call)
        {
            this->get_log_callback = dynamic_cast<core::DirectDataWriterCall*>(&call)->GetCallback();

            return true;
        }

        bool implicit_topology::get_performance_cb_callback(core::Call& call)
        {
            this->get_performance_callback = dynamic_cast<core::DirectDataWriterCall*>(&call)->GetCallback();

            return true;
        }

        bool implicit_topology::start_computation_callback(core::param::ParamSlot& slot)
        {
            // Initialize computation object
            if (!initialize_computation())
            {
                slot.ResetDirty();
                return false;
            }

            // Start computation with current values
            this->computation->start(this->num_integration_steps.Param<core::param::IntParam>()->Value(),
                this->refinement_threshold.Param<core::param::FloatParam>()->Value(),
                this->refine_at_labels.Param<core::param::BoolParam>()->Value(),
                this->distance_difference_threshold.Param<core::param::FloatParam>()->Value(),
                this->num_particles_per_batch.Param<core::param::IntParam>()->Value(),
                this->num_integration_steps_per_batch.Param<core::param::IntParam>()->Value());

            this->last_result = this->computation->get_results();

            this->computation_running = true;

            vislib::sys::Log::DefaultLog.WriteInfo("Computation of topology started...");

            // Set parameters to read-only
            set_readonly_variable_parameters(true);

            return true;
        }

        bool implicit_topology::stop_computation_callback(core::param::ParamSlot&)
        {
            // Terminate computation
            if (this->computation != nullptr && this->computation_running)
            {
                this->computation->terminate();

                vislib::sys::Log::DefaultLog.WriteInfo("Computation of topology terminated!");
            }

            this->computation_running = false;

            // Reset parameters to read-write
            set_readonly_variable_parameters(false);

            return true;
        }

        bool implicit_topology::reset_computation_callback(core::param::ParamSlot&)
        {
            // Terminate earlier computation
            stop_computation_callback();

            this->computation = nullptr;
            this->previous_result = nullptr;

            // Reset parameters to read-write
            set_readonly_fixed_parameters(false);
            set_readonly_variable_parameters(false);

            return true;
        }

        bool implicit_topology::load_computation_callback(core::param::ParamSlot& slot)
        {
            // Get load callback
            auto* call = this->result_reader_slot.CallAs<implicit_topology_reader_call>();

            if (call != nullptr && (*call)(0))
            {
                // Reset computation
                reset_computation_callback();

                // Load previous results
                implicit_topology_results previous_results;

                if (!call->GetCallback()(previous_results))
                {
                    slot.ResetDirty();
                    return false;
                }

                // Load input from file
                std::array<int, 2> resolution;
                std::array<float, 4> domain;

                std::vector<float> positions;
                std::vector<float> vectors;
                std::vector<float> points;
                std::vector<int> point_ids;
                std::vector<float> lines;
                std::vector<int> line_ids;

                if (!load_input(resolution, domain, positions, vectors, points, point_ids, lines, line_ids))
                {
                    slot.ResetDirty();
                    return false;
                }
                    
                // Create new computation object
                this->computation = std::make_unique<implicit_topology_computation>(this->get_log_callback(), this->get_performance_callback(),
                    std::move(resolution), std::move(domain), std::move(positions), std::move(vectors), std::move(points), std::move(point_ids),
                    std::move(lines), std::move(line_ids), previous_results);

                set_readonly_fixed_parameters(true);

                vislib::sys::Log::DefaultLog.WriteInfo("Previous computation of topology loaded from file.");
            }
            else
            {
                vislib::sys::Log::DefaultLog.WriteWarn("Cannot load previous results. Loader module not connected!");
            }

            return true;
        }

        bool implicit_topology::save_computation_callback(core::param::ParamSlot& slot)
        {
            if (this->computation_running)
            {
                vislib::sys::Log::DefaultLog.WriteWarn("Results can only be saved after the computation has finished.");

                slot.ResetDirty();
                return false;
            }

            if (this->previous_result == nullptr)
            {
                vislib::sys::Log::DefaultLog.WriteWarn("There is no result to write to file.");

                slot.ResetDirty();
                return false;
            }

            if (!this->get_result_writer_callback(*this->previous_result))
            {
                slot.ResetDirty();
                return false;
            }

            vislib::sys::Log::DefaultLog.WriteInfo("Previous computation of topology saved to file.");

            return true;
        }
    }
}
