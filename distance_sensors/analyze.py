#!/usr/bin/env python

# TODO: 50 -> 50+ in distance error legend
# TODO: reverse angle from sensor in PlotTargetPolarVsSensorsError.right?

"""Analysis of HC-SR04 distance sensors. Produces plots, video frames and data.

Relates recorded sensor readings to reference target location data. Specifically:

* Joins the two sources of data: sensors (actual) and scene info (reference).
* Compares the actual readings against reference locations / expected readings.
  (expected distance reading is the distance between the sensor's echo piece
   and the sensor target, computed from the target's location)
* Visualizes the comparison / intersection in various dimensions, on separate
  plots.

This is the code for analyzing the data from the experiment described in
README.md#TODO.

TODO: video frames

Sample usage:

$ distance_sensors/analyze.py  --action=PlotTargetXYVsSensorsDistance

Depending on the selected --action, the output is a plot or video frames.
See --out for output destination.

See flags for more options.
"""

import contextlib
import datetime
import itertools
import os.path
import sys
import tempfile

import cached_property
import gflags
import matplotlib.cm
import matplotlib.colors 
import numpy as np
import pandas as pd

from gimp import executor
from gimp import pdb_extra
from gimp import script_builder
from lib import linear_function
from lib import matplotlib_extra as mpl_e
from lib import numpy_extra as np_e
from lib import pandas_extra as pd_e

from video_analysis import geometry
from video_analysis import videos
from videos._20200401_145138 import metadata as video
from videos._20200401_145138 import robot

gflags.DEFINE_enum('action', None, [
  'PlotTargetXYVsSensorsDistance',
  'PlotTargetXYVsSensorsError',
  'PlotTargetPolarVsSensorsError',
  'PlotTargetAngleVsSensorsDistance',
  'PlotSensorsDistanceVsTargetDistance',
  'PlotSensorsDistanceVsErrorDistribution',
  'PlotSensorsDistanceVsCumulativeErrorDistribution',
  'PlotTargetAngleVsErrorDistribution',
  'PrintAllData',
  'DrawDistanceSensorsReadings',
# TODO: x=target angle y=distance error c=?
# TODO: x=target distance y=distance error range
], 'The action to perform / analysis to produce.')
gflags.DEFINE_string('out', None, (
  'Output path for a plot / output directory for video frames. '
  'If not given, plots are drawn on the screen, video frames are written to '
  'a temporary directory.'))
gflags.DEFINE_list('sensors', None, (
  'Sensor(s) to restrict the analysis to. '
  'If not given, defaults to all sensors in the recording of sensor readings.'))
gflags.DEFINE_enum('orientation', None, ['horizontal', 'vertical'], (
  'Plot orientation. If given, overrides plot-specific orientation.'))
gflags.DEFINE_list(
  'annotate', None,  # 'robot_time', 'frame_id', 'value',
  'If given, each point on a scatter plot is annotated with given information')

gflags.DEFINE_float('distance_error_tolerance_mm', 10, (
  'Minimal distance reading error really considered an error. '
  'Affects distance error function.'))
gflags.DEFINE_float('distance_error_max_mm', 50, (
  'Distance reading error considered maximum error. No distiction among larger '
  'erros is made, all give maximum value of distance error function.'))
gflags.DEFINE_float('target_distance_max_mm', 2000, (
  'Maximum distance reading caused by an object other than the stationary '
  'scene boundary. A larger reading indicates no object in sensor\'s field '
  'of view.'))

gflags.DEFINE_float('sensor_distance_min_mm', None, '')
gflags.DEFINE_float('sensor_distance_max_mm', None, '')
gflags.DEFINE_float('sensor_angle_min_deg', None, '')
gflags.DEFINE_float('sensor_angle_max_deg', None, '')

gflags.DEFINE_integer('error_bars_num_frames', 200, (
  'Number of preceding frames to draw bars for in each frame\'s '
  'sliding bar plot.'))

gflags.DEFINE_integer('begin_frame_id', 2520, (
  'Lower end of the reference video frame range to analyze.'))
gflags.DEFINE_integer('end_frame_id', 6624, (
  'Upper end of the reference video frame range to analyze.'))

FLAGS = gflags.FLAGS


def main():
  FLAGS(sys.argv)
  _DefineGlobals(FLAGS)
  analysis = Analysis.Create()
  getattr(analysis, FLAGS.action)()


class Analysis(object):
  """Encapsulates the module's functionality in a class. See module doc."""

  @classmethod
  def Create(cls):
    """Creates an instance with data from the particular said experiment."""
    return cls(
      robot.ReadSensorsDistanceReadings(),
      robot.ReadSensorsTargetLocations(),
      GetFrameIds(), FLAGS.sensors)

  def __init__(self, distances_sensors, target_locations, frame_ids,
               sensors=None):
    """Constructor.

    Args:
      distances_sensors: pd.DataFrame,
        index: robot time, columns: distance readings.
        Distance readings from sensors; actual readings.
      target_locations: pd.DataFrame,
        index: video frame id, columns: x, y, radius.
        Coordinates of the sensors' reference target in the robot's XY space.
        The target's projection on the robot's XY plane is an ellipse.
      frame_ids: pd.Series. Set of reference video frames to analyze.
      sensors: list of string. Sensor(s) to restrict the analysis. Optional.
    """
    self._distances_sensors = distances_sensors
    # Map the recording of the reference target's locations to the robot's
    # time space, frame id -> robot time.
    self._target_locations = self._IndexByRobotTime(
      target_locations, frame_ids=target_locations.index)
    self._frame_ids = self._IndexByRobotTime(frame_ids, frame_ids=frame_ids)
    self._sensors_all = distances_sensors.columns.tolist()
    self._sensors = sensors or self._sensors_all

    # Construct an interpolation of the reference target locations in between
    # video frames, so that its approximate location is known at the precise
    # time of each distance reading.
    loc = self._target_locations
    self._InterpolateTargetLocation = (
      linear_function.PieceWiseLinearFunction.Fit(
        x=loc.index.map(lambda robot_ts: robot_ts.total_seconds()),
        y=loc.values))

  @property
  def _begin_frame_id(self):
    """Lower end of the reference video frame range to analyze."""
    return self._frame_ids[0]

  @property
  def _end_frame_id(self):
    """Upper end of the reference video frame range to analyze."""
    return self._frame_ids[-1] + 1

  def PlotTargetXYVsSensorsDistance(self):
    """See _PlotTargetXYVsSensorDistance. Plots one for each sensor."""
    with self._PlotForEachDataFilter(self._PlotTargetXYVsSensorDistance) as fig:
      if (FLAGS.sensor_distance_min_mm is not None
          and FLAGS.sensor_distance_max_mm is not None):
        kwargs = dict(
          slice_ = slice(FLAGS.sensor_distance_min_mm,
                         FLAGS.sensor_distance_max_mm),
          ticks=[FLAGS.sensor_distance_min_mm, FLAGS.sensor_distance_max_mm])
      else:
        slice_ = None
        kwargs = dict(label='sensor reading [mm]', extend='max')
      self._PlotColorBar(
        fig, _DISTANCE_TO_COLOR.cmap, _DISTANCE_TO_COLOR.norm, **kwargs)

  def PlotTargetXYVsSensorsError(self):
    """See _PlotTargetXYVsSensorError. Plots one for each sensor."""
    with self._PlotForEachDataFilter(self._PlotTargetXYVsSensorError) as fig:
      self._PlotColorBar(
        fig, _ERROR_TO_COLOR.cmap, _ERROR_TO_COLOR.norm,
        label='reading error [mm]', extend='max')

  def PlotTargetPolarVsSensorsError(self):
    """See _PlotTargetPolarVsSensorError. Plots one for each sensor."""
    return self._PlotForEachDataFilter(
      self._PlotTargetPolarVsSensorError,
      cmap=_ERROR_CMAP, norm=_ERROR_NORM())

  def PlotTargetAngleVsSensorsDistance(self):
    """See _PlotTargetAngleVsSensorDistance. Plots one for each sensor."""
    return self._PlotForEachDataFilter(
      self._PlotTargetAngleVsSensorDistance,
      cmap=_ERROR_CMAP, norm=_ERROR_NORM(),
      orientation='horizontal')

  def PlotSensorsDistanceVsTargetDistance(self):
    """See _PlotSensorDistanceVsTargetDistance. Plots one for each sensor."""
    with self._PlotForEachDataFilter(
        self._PlotSensorDistanceVsTargetDistance) as fig:
      self._PlotColorBar(
        fig, _ERROR_TO_COLOR.cmap, _ERROR_TO_COLOR.norm,
        label='reading error [mm]', extend='max')

  def PlotSensorsDistanceVsErrorDistribution(self):
    """See _PlotSensorsDistanceVsErrorDistribution."""
    with self._PlotForEachDataFilter(
        self._PlotSensorDistanceVsErrorDistribution) as fig:
      pass

  def PlotSensorsDistanceVsCumulativeErrorDistribution(self):
    """See _PlotSensorsDistanceVsErrorDistribution."""
    with self._PlotForEachDataFilter(
        self._PlotSensorDistanceVsCumulativeErrorDistribution) as fig:
      pass

  def PlotTargetAngleVsErrorDistribution(self):
    """See _PlotTargetAngleVsErrorDistribution."""
    with self._PlotForEachDataFilter(
        self._PlotTargetAngleVsErrorDistribution) as fig:
      pass

  def DrawDistanceSensorsReadings(self):
    """Visualizes (superposes) sensor readings and errors on video frames."""
    if FLAGS.out:
      assert os.path.isdir(FLAGS.out)
      frame_dir = FLAGS.out
    else:
      frame_dir = tempfile.mkdtemp()
      print 'Writing video frames to ' + frame_dir

    video_frame_provider = video.GetVideoFrameProvider(format_='png')
    # # video.EXTRACTED_FRAMES_DIR
    
    # if FLAGS.video_frames:
    #   frame_png_source = videos.ExtractedFrames(FLAGS.video_frames).GetPNG
    # else:
    #   frame_png_source = videos.ExtractedFrames(FLAGS.video_frames).GetPNG

    draw_script = script_builder.GIMPScriptBuilder()
    pdb_e = pdb_extra.PDBExtra(draw_script.pdb)

    # Preload sensor masks.
    pdb_e.OpenImage(robot.SENSORS_MASK_IMAGE_PATH)
    draw_script.AddNativeCode(self._GET_SENSOR_MASK_CODE)  # TODO: Clean up.

    for frame_id in self._frame_ids:
      with pdb_e.TransformImage(
          path=video_frame_provider(frame_id),
          out_path='%s/%04u.png' % (frame_dir, frame_id)) as frame:
        self._DrawDistanceSensorsReadingsInFrame(frame_id, frame, pdb_e)

    gimp = executor.GIMPBatchInterpreterScriptExecutor()
    gimp.ExecuteScript(draw_script.Build())
      
  # def _DrawDistanceSensorsReadingsInFrame(self, frame_id, frame, pdb_e):
  #   pdb = pdb_e._pdb
  #   pdb._builder.AddNativeCode(
  #     'print %s.uri' % frame._image._ToNativeCode())  # TODO: Clean up.
  #   pdb.gimp_context_set_opacity(30)

  #   for sensor_id in self._sensors:
  #     distance_error = ...
  #     pdb.gimp_context_set_foreground(color)

  #     color = self._DistanceErrorToColor(distance_error)  # TODO
  #     receptive_area, distance_error = (
  #       self._GetDistanceSensorReceptiveArea(frame_id, sensor_id))
  #     receptive_area_polygon = (
  #       receptive_area.ToVectorSpace(robot.VIDEO_PX).ToPolygon(num_segments=20))

  #     # Draw sensor reading in the video frame, color depending on sensor error.
  #     pdb_e.PaintPolygon(frame, receptive_area_polygon)

  #     # Colorize the sensor echo piece with the same color.
  #     GetSensorMask = pdb._builder.Global('GetSensorMask')  # TODO: Clean up.
  #     pdb_e.Colorize(frame, mask=GetSensorMask(sensor_id))

  #   for sensor_id, y_high in zip(self._sensors, (450, 550, 650)):
  #     # Overlay a sliding bar plot of sensor error.
  #     num_frames = FLAGS.error_bars_num_frames
  #     distance_errors = (
  #       self._distance_errors[sensor_id][frame_id-num_frames:frame_id])
  #     error_bar_heights = self._DistanceErrorToErrorBarHeight(distance_errors)
  #     y_high = np.full(num_frames, y_high)
  #     pdb_e.DrawBars(
  #       frame, x=np.linspace(550, 550 + 2*num_frames, num_frames+1),
  #       y_low=y_high - error_bar_heights * 50, y_high=y_high,
  #       color=self._DistanceErrorsToColor(distance_errors),  # TODO
  #       border=_ToRGB('white'))

  _GET_SENSOR_MASK_CODE = """
SENSORS_MASK_IMAGE = gimp.image_list()[0]
assert SENSORS_MASK_IMAGE.uri == 'file://%(sensor_masks_xcf_path)s'

def GetSensorMask(sensor_id):
  for c in SENSORS_MASK_IMAGE.channels:
    if c.name == sensor_id:
      return c
  raise Exception()
""" % dict(sensor_masks_xcf_path=os.path.abspath(robot.SENSORS_MASK_IMAGE_PATH))

  def _GetDistanceSensorReceptiveArea(self, frame_id, sensor_id):
    robot_time = self._FrameIdToRobotTime(frame_id)
    sensor_echo_xy = robot.FLOOR_MM.SENSOR_XY['%s_echo' % sensor_id].p1
    sensor_distance = self._all_data[sensor_id].asof(robot_time)
    receptive_area = robot.FLOOR_MM.EllipseArc(
      robot.FLOOR_MM.Ellipse(sensor_echo_xy, sensor_distance),
      *robot.FLOOR_MM.SENSOR_ANGLE_RANGE[sensor_id])

    if self._IsReadingOfTarget(sensor_distance):
      target_distance = self._all_data.loc[robot_time, sensor_id + '_distance']
      distance_error = self._DistanceError(sensor_distance, target_distance)
    else:
      distance_error = np.nan

    return receptive_area, distance_error

  @contextlib.contextmanager
  def _PlotForEachDataFilter(self, plot_func, orientation='horizontal'):
    data_filters = self._GetDataFilters()
    if data_filters.ndim == 1:
      data_filters = data_filters.reshape(1, -1)

    # if orientation == 'vertical':
    #   if data_filters.shape[0] < data_filters.shape[1]:
    #     data_filters = data_filters.T
    # elif orientation == 'horizontal':
    #   if data_filters.shape[0] > data_filters.shape[1]:
    #     data_filters = data_filters.T
    # else:
    #   raise ValueError(orientation)

    with mpl_e.SubPlots(
        nrows=data_filters.shape[0], ncols=data_filters.shape[1], sharey=True,
        extra=dict(tight=True, path=FLAGS.out)) as (fig, axs):
      for data_filter, ax in zip(data_filters.ravel(), axs.ravel()):
        ax.ax.set_title(str(data_filter))
        plot_func(data_filter, ax)
      yield fig

  def _GetDataFilters(self):
    assert len(FLAGS.sensors) == 1
    data_filters = [DataFilter(FLAGS.sensors[0])]

    if FLAGS.sensor_angle_min_deg or FLAGS.sensor_angle_max_deg:
      angle_min = geometry.DegreesToRadians(FLAGS.sensor_angle_min_deg or 0)
      angle_max = geometry.DegreesToRadians(FLAGS.sensor_angle_max_deg or 360)
      data_filters.append(
        DataFilter(FLAGS.sensors[0], angle_min, angle_max))

    return np.array(data_filters)

  def _PlotColorBar(self, fig, *args, **kwargs):
    mpl_e.ColorBar(fig, fraction=0.1, pad=0.05, aspect=40, *args, **kwargs)

  def _AnnotateScatterPlotSensors(self, values, x, y, ax):
    """

    Args:
      values: pd.Series, index: robot timestamps, values: any.
    """
    robot_timestamps = values.index

    text = [[] for _ in x]
    annotate = dict.fromkeys(FLAGS.annotate or (), True)
    if annotate.pop('robot_time', False):
      for i in range(0, len(text), 15):
        text[i].append('%u' % robot_timestamps[i].total_seconds())  # TODO: Format.
    if annotate.pop('frame_id', False):
      for t, rt in zip(text, robot_timestamps):
        frame_id, is_exact = self._RobotTimeToFrameId(rt)
        t.append('%u%s' % (frame_id, '* '[is_exact]))
    if annotate.pop('value', False):
      for t, v in zip(text, values):
        if not np.isnan(v):
          t.append(('%u' % v))
    assert not annotate
    
    ax.extra.Annotate(x=x, y=y, text=[' '.join(t) for t in text])

  def _PlotTargetXYVsSensorDistance(self, data_filter, ax):
    """Plots sensor readings in relation to the sensor target's locations.

    Plots a scatter plot, one point for each sensor reading:
      x, y = sensor target location in the robot's XY plane
      color = sensor distance reading
    """
    if (FLAGS.sensor_distance_min_mm is not None
        and FLAGS.sensor_distance_max_mm is not None):
      distance_lines = [
        FLAGS.sensor_distance_min_mm, FLAGS.sensor_distance_max_mm]
    else:
      distance_lines = np.linspace(0, FLAGS.target_distance_max_mm, 10+1)
    sensor_echo_xy = (
      robot.FLOOR_MM.SENSOR_XY['%s_echo' % data_filter.sensor].p1.xy)
    for distance in distance_lines:
      ax.extra.Circle(
        sensor_echo_xy, distance, color=_DISTANCE_TO_COLOR(distance),
        fill=False)
    
    sensor_distances, target_locations = self._GetData(data_filter)[:2]

    if (FLAGS.sensor_distance_min_mm is not None
        and FLAGS.sensor_distance_max_mm is not None):
      is_within_distance = lambda d: (
        FLAGS.sensor_distance_min_mm <= d < FLAGS.sensor_distance_max_mm)
    else:
      is_within_distance = None

    for (x, y, r), sensor_distance in zip(target_locations, sensor_distances):
      if is_within_distance is None or is_within_distance(sensor_distance):
        color = _DISTANCE_TO_COLOR(sensor_distance)
        alpha = 0.1
      else:
        color = 'gray'
        alpha = 0.02
      ax.extra.Circle((x, y), r, color=color, alpha=alpha)

    self._PlotArena(ax)
    self._PlotSensors(data_filter.sensor, ax)

    # xy = target_locations[:, :2].T
    # self._AnnotateScatterPlotSensors(sensor_distances, *xy, ax=ax)

  def _PlotTargetXYVsSensorError(self, data_filter, ax):
    """Plots sensor reading error in relation to the sensor target's location.

    Plots a scatter plot, one point for each sensor reading:
      x, y = sensor target location in the robot's XY plane
      color = distance error, difference between sensor and expected reading,
              scaled to [0, 1] distance error function; empty readings are gray
    """
    _, target_locations, _, distance_errors = self._GetData(data_filter)
    
    for (x, y, r), distance_error in zip(target_locations, distance_errors):
      if not np.isnan(distance_error):
        color = _ERROR_TO_COLOR(distance_error)
        alpha = 0.1
      else:
        color = 'gray'
        alpha = 0.02
      ax.extra.Circle((x, y), r, color=color, alpha=alpha)

    self._PlotArena(ax)
    self._PlotSensors(data_filter.sensor, ax)
    self._AnnotateScatterPlotSensors(
      distance_errors, *target_locations[:, :2].T, ax=ax)

  def _PlotTargetPolarVsSensorError(self, data_filter, ax):
    """Plots sensor reading error in relation to the sensor target's location.

    Plots a scatter plot, one point for each sensor reading:
      y = distance between the sensor echo piece and the sensor target
      x = angle from the sensor's echo piece to the sensor target
      color = distance error, difference between sensor and expected reading,
              scaled to [0, 1] distance error function; empty readings are gray
    """
    _, _, target_distangles, distance_error, is_target_reading = (
      self._GetData(data_filter))

    distances, angles = target_distangles.T
    angles_deg = geometry.RadiansToDegrees(angles)
    x, y = angles_deg, distances
    ax.extra.ScatterLegend(
      x=x[is_target_reading], y=y[is_target_reading],
      c=distance_error[is_target_reading],
      s=3, legend=dict(
        title='distance\nerror',
        c_levels=np.linspace(0, FLAGS.distance_error_max_mm, 11),
        c_format='%u'))
    ax.ax.scatter(
      x=x[~is_target_reading], y=y[~is_target_reading], c='gray', s=3)
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('angle from sensor wrt vertical x=0 [deg]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('distance from sensor [mm]')
    ax.ax.invert_xaxis()
    return x, y
    
  def _PlotTargetAngleVsSensorDistance(self, data_filter, ax):
    """Plots sensor reading error in relation to the sensor target's direction

    Plots a scatter plot, one point for each sensor reading:
      y = sensor distance reading
      x = angle from the sensor's echo piece to the sensor target
      color = distance error, difference between sensor and expected reading,
              scaled to [0, 1] distance error function; empty readings are gray
    """
    sensor_distances, _, target_distangles, distance_error, is_target_reading = (
      self._GetData(data_filter))

    distances, angles = target_distangles.T
    angles_deg = geometry.RadiansToDegrees(angles)
    x, y = angles_deg, sensor_distances
    ax.extra.ScatterLegend(
      x=x[is_target_reading], y=y[is_target_reading],
      c=distance_error[is_target_reading],
      s=3, legend=dict(
        title='distance\nerror',
        c_levels=np.linspace(0, FLAGS.distance_error_max_mm, 11),
        c_format='%u'))
    ax.ax.scatter(
      x=x[~is_target_reading], y=y[~is_target_reading], c='gray', s=3) 
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('angle from sensor wrt vertical x=0 [deg]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('distance reading [mm]')
    ax.ax.invert_xaxis()
    return x, y

  def _PlotSensorDistanceVsTargetDistance(self, data_filter, ax):
    side = FLAGS.target_distance_max_mm
    ax.ax.set_xlim(0, side)
    ax.ax.set_ylim(0, side)
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('distance reading [mm]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('target distance [mm]')
    ax.ax.plot(
      [0, FLAGS.target_distance_max_mm], [0, FLAGS.target_distance_max_mm],
      color='black', linewidth=0.5, antialiased=True)

    sensor_distances, _, target_distangles, distance_error = (
      self._GetData(data_filter))
    is_target_reading = ~np.isnan(distance_error.values)

    for sensor_distance, (target_distance, angle), error in zip(
        sensor_distances[is_target_reading],
        target_distangles[is_target_reading],
        distance_error[is_target_reading]):
      ax.ax.scatter(
        x=sensor_distance, y=target_distance,
        c=_ERROR_TO_COLOR(error),
        marker=((0, 0), (np.sin(angle), -np.cos(angle))),
        s=50)

  def _PlotSensorDistanceVsErrorDistribution(self, data_filter, ax):
    sensor_distances, _, _, distance_error  = self._GetData(data_filter)
    is_target_reading = ~np.isnan(distance_error)
    sensor_distances = sensor_distances[is_target_reading]
    distance_error = distance_error[is_target_reading]

    # distance_interval_bounds = np.arange(0, sensor_distances.max() + 50, 50)
    distance_interval_bounds = np.arange(0, 1900, 50)
    num_di = len(distance_interval_bounds) - 1
    interval_ids = np.digitize(sensor_distances, distance_interval_bounds)

    boxes = ax.ax.boxplot(
      np_e.GroupByIndicesRange(distance_error, interval_ids, 0, num_di),
      positions=np.arange(num_di) + 0.5, manage_xticks=False,
      widths=1, whis=(0, 100), patch_artist=True)['boxes']
    for b in boxes:
      b.set_facecolor('gray')
    xticks = np.arange(0, num_di, 5)
    ax.ax.set_xticks(xticks)
    ax.ax.set_xticklabels(map(str, [i*50*5 for i in range(len(xticks))]))
    ax.ax.set_ylim(0, 500)
    yticks = np.linspace(0, 500, 10+1).astype(np.int)
    ax.ax.set_yticks(yticks)
    ax.ax.set_yticklabels(map(str, yticks))
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('distance reading [mm]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('distance error [mm]')

  def _PlotSensorDistanceVsCumulativeErrorDistribution(self, data_filter, ax):
    sensor_distances, _, _, distance_error  = self._GetData(data_filter)
    is_target_reading = ~np.isnan(distance_error)
    sensor_distances = sensor_distances[is_target_reading]
    distance_error = distance_error[is_target_reading]
    
    x = []
    distances_errors = np_e.SortedByColumn(
      np.array([sensor_distances, distance_error]).T, 0)
    for unique_distance_end_index in itertools.chain(
        np.unique(distances_errors[:, 0], return_index=True)[1][1:],
        [len(distances_errors)]):
      distance = distances_errors[unique_distance_end_index - 1, 0]
      errors = distances_errors[:unique_distance_end_index, 1]
      error_quantiles = np.percentile(errors, self._QUANTILES * 100)
      x.append((distance, error_quantiles))
    xx = pd.DataFrame(
      [_[1] for _ in x], index=[_[0] for _ in x],
      columns=self._QUANTILES)

    ylim = FLAGS.distance_error_max_mm * 2  # TODO
    ax.ax.plot(xx.index, xx, color='black', linewidth=1)
    ax.ax.plot(xx.index, xx[0.50], color='#ff7f0e', linewidth=1)
    ax.ax.fill_between(xx.index, xx[0.25], xx[0.75], color='#808080')
    d = xx.loc[500:1000][0.75].idxmin()
    ax.extra.Annotate(
      [d] * len(self._QUANTILES),
      np.minimum(xx.loc[d].values + 1.5, ylim * 1.1),
      ['%u%%' % q for q in (self._QUANTILES * 100)],
      color=['black' if q != 0.50 else '#ff7f0e' for q in self._QUANTILES],
      ha='center')

    ax.ax.set_xlim(0, distance)
    ax.ax.set_ylim(0, ylim)
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('distance reading [mm]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('distance error [mm]')

  _QUANTILES = np.array([0.00, 0.25, 0.50, 0.75, 0.90, 0.95, 1.00])

  def _PlotTargetAngleVsErrorDistribution(self, data_filter, ax):
    _, _, target_distangles, distance_error = self._GetData(data_filter)
    is_target_reading = ~np.isnan(distance_error)
    target_angles = target_distangles[is_target_reading, 1]
    target_angles = geometry.RadiansToDegrees(target_angles)
    distance_error = distance_error[is_target_reading]

    angle_interval_bounds = ai = np.arange(
      _PrevMultiple(target_angles.min(), 5),
      _NextMultiple(target_angles.max(), 5) + 1, 5)
    num_ai = len(ai) - 1
    interval_ids = np.digitize(target_angles, angle_interval_bounds)

    boxes = ax.ax.boxplot(
      np_e.GroupByIndicesRange(distance_error, interval_ids, 0, num_ai),
      positions=np.arange(num_ai) + 0.5, manage_xticks=False,
      widths=1, whis=(0, 100), patch_artist=True)['boxes']
    for b in boxes:
      b.set_facecolor('gray')
    # print robot.FLOOR_MM.SENSOR_ANGLE[sensor_id]
    # print geometry.RadiansToDegrees(
    #   robot.FLOOR_MM.SENSOR_ANGLE[sensor_id])
    # ax.ax.axvline(x=geometry.RadiansToDegrees(
    #   robot.FLOOR_MM.SENSOR_ANGLE[sensor_id]))
      
    ax.ax.set_xticks(np.arange(0, len(angle_interval_bounds)))
    ax.ax.set_xticklabels(map(str, angle_interval_bounds))
    for txt in ax.ax.xaxis.get_majorticklabels():
      txt.set_rotation(90)
    ax.ax.set_ylim(0, 500)
    yticks = np.linspace(0, 500, 10+1).astype(np.int)
    ax.ax.set_yticks(yticks)
    ax.ax.set_yticklabels(map(str, yticks))
    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('angle between vertical x=0 and sensor-to-target [deg]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('distance error [mm]')
    ax.ax.invert_xaxis()

  def _PlotArena(self, ax):
    xy = robot.FLOOR_MM.ARENA.PointsArray()
    xy = np.vstack([xy, xy[0]])
    ax.ax.plot(*xy.T, color='black', linewidth=0.5, antialiased=True)

    if ax.extra.is_bottommost:
      ax.ax.set_xlabel('x [mm]')
    if ax.extra.is_leftmost:
      ax.ax.set_ylabel('y [mm]')
    x_min, y_min = xy.min(axis=0)
    x_max, y_max = xy.max(axis=0)
    ax.ax.set_xlim(x_min, x_max)
    ax.ax.set_ylim(y_min, y_max)
    ax.ax.invert_yaxis()

  def _PlotSensors(self, sensor_id, ax):
    sensor_echo_xy = [robot.FLOOR_MM.SENSOR_XY['%s_echo' % s].p1.xy
                      for s in self._sensors_all]
    ax.ax.scatter(
      *np.array(sensor_echo_xy).T,
      c=['black' if s == sensor_id else 'lightgray' for s in self._sensors_all],
      marker='x', s=5)

    echo_segment = robot.FLOOR_MM.SENSOR_XY['%s_echo' % sensor_id]
    mpl_e.Arrow(echo_segment.p1.xy, echo_segment.p2.xy,
                color='black', linewidth=0.5)

  def PrintAllData(self):
    """Prints the analysis' complete, raw data, joined into a single dataframe.

    The data is:
      * robot time (index)
      * video frame time
      * video frame id
      * sensor distance readings
      * sensor target locations in xy coordinates
      * sensor target locations in polar coordinates from each sensor's echo piece
    """
    time_slice = slice(
      self._FrameIdToRobotTime(self._begin_frame_id),
      self._FrameIdToRobotTime(self._end_frame_id))
    print self._all_data[time_slice].to_string(formatters={
      c: (pd_e.FormatInt if c in ['frame_id'] + self._sensors
          else pd_e.FormatFloatShort)
      for c in self._all_data.columns if c != 'frame_time'})
      # 'frame_id': _FormatInt,
      # 'left_angle_rad': _FormatFloatShort,
      # 'front_angle_rad': _FormatFloatShort,
      # 'right_angle_rad': _FormatFloatShort,
      # 'left_distance': _FormatFloatShort,
      # 'front_distance': _FormatFloatShort,
      # 'right_distance': _FormatFloatShort,
      # 'x': _FormatFloatShort,
      # 'y': _FormatFloatShort,
      # 'radius_x': _FormatFloatShort,
      # 'radius_y': _FormatFloatShort,
      # 'left': _FormatInt,
      # 'front': _FormatInt,
      # 'right': _FormatInt})

  @cached_property.cached_property
  def _all_data(self):
    # Start with frame_time and frame_id, indexed by robot time.
    indices = pd.DataFrame(dict(
      frame_id=self._frame_ids,
      frame_time=map(video.FrameIdToFrameTime, self._frame_ids)
    ))[['frame_id', 'frame_time']]
    # Add polar distances + angles from each sensor to target locations.
    loc = self._target_locations.copy()
    for sensor in self._sensors:
      loc[sensor + '_distance'], loc[sensor + '_angle_rad'] = (
        self._GetTargetDistangles(sensor, self._target_locations.values).T)
    return pd.concat([indices, loc, self._distances_sensors], axis=1)

  @cached_property.cached_property
  def _distance_errors(self):
    distance_errors = {}
    begin = self._begin_frame_id - FLAGS.error_bars_num_frames
    end = self._end_frame_id
    for sensor_id in self._sensors:
      distance_errors[sensor_id] = np.empty(self._end_frame_id)
      distance_errors[sensor_id][begin:end] = np.array([
        self._GetDistanceSensorReceptiveArea(frame_id, sensor_id)[1]
        for frame_id in range(begin, end)])
    return distance_errors

  def _IndexByRobotTime(self, df_or_array_like, frame_ids):
    robot_times = map(self._FrameIdToRobotTime, frame_ids)
    if isinstance(df_or_array_like, pd.DataFrame):
      df = df_or_array_like
      return pd.DataFrame(df.values, columns=df.columns, index=robot_times)
    else:
      return pd.Series(df_or_array_like, index=robot_times)

  def _RobotTimeToFrameId(self, robot_timestamp):
    return pd_e.asof_is_exact(self._frame_ids, robot_timestamp)

  def _FrameIdToRobotTime(self, frame_id):
    if not hasattr(self, '_frame_time_to_robot_time'):
      self._frame_time_to_robot_time =  robot.FrameTimeToRobotTime.Fit()
    frame_time = video.FrameIdToFrameTime(frame_id)
    robot_time = self._frame_time_to_robot_time(frame_time)
    return robot_time

  def _GetTargetLocations(self, robot_timestamps):
    if isinstance(robot_timestamps, datetime.timedelta):
      robot_seconds = robot_timestamps.total_seconds()
    else:
      robot_seconds = [rt.total_seconds() for rt in robot_timestamps]
    return self._InterpolateTargetLocation(robot_seconds)

  def _GetTargetDistangles(self, sensor_id, target_locations):
    sensor_echo_xy = robot.FLOOR_MM.SENSOR_XY['%s_echo' % sensor_id].p1
    return np.array([
      robot.FLOOR_MM.Circle.FromCoords(x, y, r)
      .GetClosestPoint(sensor_echo_xy).ToPolar(origin=sensor_echo_xy)
      for x, y, r in target_locations])

  def _GetData(self, data_filter):
    target_locations_time_slice = slice(
      self._target_locations.index[0],
      self._target_locations.index[-1])

    sensor_distances = (
      self._distances_sensors[data_filter.sensor].dropna()
      [target_locations_time_slice])
    target_locations = (
      self._GetTargetLocations(sensor_distances.index))
    target_distangles = (
      self._GetTargetDistangles(data_filter.sensor, target_locations))
    distance_error = (
      self._DistanceError(sensor_distances, target_distangles[:, 0]))
    is_target_reading = self._IsReadingOfTarget(sensor_distances)
    distance_error = np.choose(is_target_reading, (np.nan, distance_error))

    if not (data_filter.sensor_angle_min and data_filter.sensor_angle_max):
      return sensor_distances, target_locations, target_distangles, distance_error

    else:
      is_within_angle = (
        (data_filter.sensor_angle_min <= target_distangles[:, 1])
        & (target_distangles[:, 1] < data_filter.sensor_angle_max))
      return (
        sensor_distances[is_within_angle],
        target_locations[is_within_angle],
        target_distangles[is_within_angle],
        distance_error[is_within_angle])

  @staticmethod
  def _DistanceError(sensor_distance, target_distance):
    return np.abs(sensor_distance - target_distance)

  # TODO
  @staticmethod
  def _DistanceErrorToColor(distance_error):
    if distance_error:
      return _ERROR_TO_COLOR(distance_error)
    else:
      return _ToRGB('black')

  @staticmethod
  def _DistanceErrorsToColor(distance_error):
    is_in_sensor_range = ~np.isnan(distance_error)
    color = np.empty((len(distance_error), 3))
    color[is_in_sensor_range, :] = (
      _ERROR_TO_COLOR(distance_error[is_in_sensor_range]))[:, :3]
    color[~is_in_sensor_range, :] = _ToRGB('black')
    return color

  @staticmethod
  def _DistanceErrorToErrorBarHeight(distance_error):
    is_in_sensor_range = ~np.isnan(distance_error)
    error_bar_heights = np.empty(len(distance_error))
    error_bar_heights[is_in_sensor_range] = (
      _ERROR_TO_COLOR.norm(distance_error[is_in_sensor_range]))
    error_bar_heights[~is_in_sensor_range] = 1.2
    return error_bar_heights

  @staticmethod
  def _IsReadingOfTarget(sensor_distance):
    return sensor_distance <= FLAGS.target_distance_max_mm


class DataFilter(object):

  def __init__(self, sensor, sensor_angle_min=None, sensor_angle_max=None):
    self.sensor = sensor
    self.sensor_angle_min = sensor_angle_min
    self.sensor_angle_max = sensor_angle_max

  def __str__(self):
    parts = ['sensor=%s' % self.sensor]
    if self.sensor_angle_min or self.sensor_angle_max:
      parts.append('angle$\in[%u^\circ, %u^\circ]$' % (
        geometry.RadiansToDegrees(self.sensor_angle_min),
        geometry.RadiansToDegrees(self.sensor_angle_max)))
    return ', '.join(parts)


def GetFrameIds():
  begin_frame_id, end_frame_id = video.FRAME_ID_RANGE
  begin_frame_id = max(begin_frame_id, FLAGS.begin_frame_id or 0)
  if FLAGS.end_frame_id is not None:
    end_frame_id = min(end_frame_id, FLAGS.end_frame_id)
  return np.arange(begin_frame_id, end_frame_id)


def _DefineGlobals(FLAGS):
  global _DISTANCE_TO_COLOR
  _DISTANCE_TO_COLOR = mpl_e.ValueToColor.Create(
    cmap='rainbow',
    norm=matplotlib.colors.Normalize(0, FLAGS.target_distance_max_mm))

  global _ERROR_TO_COLOR
  _ERROR_TO_COLOR = mpl_e.ValueToColor(
    cmap=mpl_e.ColorMap.PieceWiseLinear(
      'green_yellow_red', [0.0, 0.5, 1.0], ('green', 'yellow', 'red')),
    norm=matplotlib.colors.Normalize(
      0, FLAGS.distance_error_max_mm, clip=True)
    # norm=mpl_e.Normalize.PieceWiseLinear([
    #   (0, 0),
    #   (FLAGS.distance_error_tolerance_mm, 0),
    #   (FLAGS.distance_error_max_mm, 1)
    # ], clip=True)
  )

  global _ANGLE_TO_COLOR
  _ANGLE_TO_COLOR = mpl_e.ValueToColor.Create(
    cmap='BrBG',
    norm=matplotlib.colors.Normalize(-np.pi/3, np.pi/3))


def _ToRGB(color):
  return matplotlib.colors.to_rgb(color)


def _PrevMultiple(x, n):
  return int(float(x) / n) * n

def _NextMultiple(x, n):
  return int(np.ceil(float(x) / n)) * n


if __name__ == '__main__':
  main()

